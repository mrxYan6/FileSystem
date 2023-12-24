#include <iostream>
#include <vector>
#include <climits>
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <string>

extern "C" {
    #include "fs.h"
}

void show_help(){
    std::cout << "命令名\t\t命令参数\t\t命令功能\n\n";
    std::cout << "cd\t\t目录名(路径名)\t\t切换当前目录到指定目录\n";
    std::cout << "mkdir\t\t目录名\t\t\t在当前目录创建新目录\n";
    std::cout << "ls\t\t无\t\t\t显示当前目录下的目录和文件\n";
    std::cout << "create\t\t文件名\t\t\t在当前目录下创建指定文件\n";
    std::cout << "rm\t\t文件名\t\t\t在当前目录下删除指定文件\n";
    std::cout << "rm\t-r\t文件名\t\t\t在当前目录下删除指定目录\n";
    std::cout << "open\t\t文件名\t\t\t在当前目录下打开指定文件\n";
    std::cout << "write\t\t无\t\t\t写该文件，如未打开，则打开，写完后关闭\n";
    std::cout << "read\t\t无\t\t\t读取该文件，如未打开，则打开\n";
    std::cout << "close\t\t无\t\t\t在打开文件状态下，关闭该文件\n";
    std::cout << "exit\t\t无\t\t\t退出系统\n\n";
}

void error(std::string command){
    std::cout << command << " : 缺少参数\n";
    std::cout << "输入 'help' 来查看命令提示.\n";
}

int main(){
    std::string cmdToken[] = {"mkdir", "rmdir", "ls", "cd", "create", "rm", "open", "close", "write", "read", "exit", "help"};

    std::cout << "\n\n************************ 文件系统 **************************************************\n";
    std::cout << "************************************************************************************\n\n";
     // 创建并初始化文件系统
    FileSystem fs;
    UserOpenTable openTable;
    tbl_init(&openTable);

    // 初始化文件系统
    std::cout << "输入保存路径:\n";
    std::string path;
    std::cin >> path;
    FILE* fp = fopen(path.c_str(), "rb");
    if (fp == NULL) {
        std::cout << "文件不存在，创建新文件系统\n";
        format(&fs);
    } else {
        loadFs(&fs, fp);
        fclose(fp);
    } 

    std::getchar();
    std::string buffer;
    while(1){
        //需要printf当前目录    !!!!!!!!!!!!!!
        std::cout << fs.current_dir_path << " > ";

        std::getline(std::cin, buffer);
        // return 0;
        std::stringstream ss;
        ss << buffer;

        std::vector<std::string> tokens;
        std::string temp;
        while (ss >> temp) {
            tokens.push_back(temp);
        }
        if (tokens.size() == 0) continue;        
        int id = -1;
        for (int i = 0; i < 12; ++i) {
            if (tokens[0] == cmdToken[i]) {
                id = i;
                break;
            }
        }

        char *sp1, *sp2, *sp3;

        switch (id) {
            case 0:         // mkdir
                if (tokens.size() != 2) {
                    std::cerr << "mkdir : 参数错误\n";
                    break;
                } else {
                    sp1 = (char*)malloc(tokens[1].length() + 1);
                    strcpy(sp1, tokens[1].c_str());
                    mkdir(&fs, sp1);
                    free(sp1);
                }
                break;
            case 1:         // rmdir
                if (tokens.size() != 2) {
                    std::cerr << "mkdir : 参数错误\n";
                    break;
                } else {
                    sp1 = (char*)malloc(tokens[1].length() + 1);
                    strcpy(sp1, tokens[1].c_str());
                    rm(&fs, sp1, 1);
                    free(sp1);
                }
                break;
            case 2:         // ls
                if (tokens.size() == 1) {
                    ls(&fs, NULL);
                } else if (tokens.size() == 2) {
                    sp1 = (char*)malloc(tokens[1].length() + 1);
                    strcpy(sp1, tokens[1].c_str());
                    ls(&fs, sp1);
                    free( sp1);
                } else {
                    std::cerr << "ls : 参数错误\n";
                }
                break;
            case 3:         // cd
                if (tokens.size() != 2) {
                    std::cerr << "cd : 参数错误\n";
                    break;
                } else {
                    sp1 = (char*)malloc(tokens[1].length() + 1);
                    strcpy(sp1, tokens[1].c_str());
                    cd(&fs, sp1);
                    free( sp1);
                }
                break;
            case 4:         // create
                if (tokens.size() != 2) {
                    std::cerr << "create : 参数错误\n";
                    break;
                } else {
                    sp1 = (char*)malloc(tokens[1].length() + 1);
                    strcpy(sp1, tokens[1].c_str());
                    create(&fs, sp1);
                    free( sp1);
                }
                break;
            case 5:         // rm
                if (tokens.size() == 2) {
                    sp1 = (char*)malloc(tokens[1].length() + 1);
                    strcpy(sp1, tokens[1].c_str());
                    rm(&fs, sp1, 0);
                    free( sp1);
                } else if(tokens.size() == 3) {
                    if (tokens[1] == "-r") {
                        sp1 = new char [tokens[2].length() + 1];
                        strcpy(sp1, tokens[2].c_str());
                        rm(&fs, sp1, 1);
                        free( sp1);
                    } else {
                        std::cerr << "rm : 参数错误\n";
                    }
                } else {
                    std::cerr << "rm : 参数错误\n";
                }
                break;
            case 6:         // open
                if (tokens.size() != 2) {
                    std::cerr << "open : 参数错误\n";
                    break;
                } else {
                    sp1 = (char*)malloc(tokens[1].length() + 1);
                    strcpy(sp1, tokens[1].c_str());
                    open(&fs, &openTable, sp1);
                    free( sp1);
                }
                break;
            case 7:         // close
                if (tokens.size() != 2) {
                    std::cerr << "close : 参数错误\n";
                    break;
                } else {
                    sp1 = (char*)malloc(tokens[1].length() + 1);
                    strcpy(sp1, tokens[1].c_str());
                    close(&fs, &openTable, sp1);
                    free( sp1);
                }
                break;
            case 8:         // write name linecnt
                if (tokens.size() == 3) {
                    // 默认截断写
                    sp1 = (char*)malloc(tokens[1].length() + 1);
                    strcpy(sp1, tokens[1].c_str());
                    int linecnt = atoi(tokens[2].c_str());
                    std::string tmp;
                    std::getline(std::cin, tmp);
                    // std::cin >> tmp;
                    // std::cout << "KLHEFIOUBWYUOEFGI!!!!\n";
                    for (int i = 1; i < linecnt; ++i) {
                        std::string tmp2;
                        // std::cin >> tmp2;
                        std::getline(std::cin, tmp2);
                        tmp = tmp + "\n" + tmp2;
                    }
                    // std::cout << "I*Y*&TQ@&^#R^&E#@&^GU!!!!\n";

                    sp2 = (char*)malloc(tmp.length() + 1);
                    strcpy(sp2, tmp.c_str());
                    write(&fs, &openTable, sp1, tmp.length() , sp2, 1);

                    free(sp2);
                    free(sp1);
                } else if (tokens.size() == 4) {
                    if (tokens[3] == "-a") {
                        // 追加
                        sp1 = (char*)malloc(tokens[1].length() + 1);
                        strcpy(sp1, tokens[1].c_str());
                        int linecnt = atoi(tokens[2].c_str());
                        std::string tmp;
                        std::getline(std::cin, tmp);
                        // std::cin >> tmp;
                        for (int i = 1; i < linecnt; ++i) {
                            std::string tmp2;
                            std::getline(std::cin, tmp2);
                            tmp = tmp + "\n" + tmp2;
                        }
                        std::cerr << "WRIT: " << tmp << "\n";
                        sp2 = (char*)malloc(tmp.length() + 1);
                        strcpy(sp2, tmp.c_str());
                        write(&fs, &openTable, sp1, tmp.length(), sp2, 2);
                        free( sp2);
                        free( sp1);
                    } else if (tokens[3] == "-r") {
                        // 覆盖
                        sp1 = (char*)malloc(tokens[1].length() + 1);
                        strcpy(sp1, tokens[1].c_str());
                        int linecnt = atoi(tokens[2].c_str());
                        std::string tmp;
                        std::cin >> tmp;
                        for (int i = 1; i < linecnt; ++i) {
                            std::string tmp2;
                            std::cin >> tmp2;
                            tmp = tmp + "\n" + tmp2;
                        }
                        sp2 = (char*)malloc(tmp.length() + 1);
                        strcpy(sp2, tmp.c_str());
                        write(&fs, &openTable, sp1, tmp.length(), sp2, 0);
                        free( sp2);
                        free( sp1);
                    } else {
                        std::cerr << "write : 参数错误\n";
                    }
                } else {
                    std::cerr << "write : 参数错误\n";
                }
                break;
            case 9:         // read
                int len;
                if (tokens.size() != 3) {
                    std::cerr << "read : 参数错误\n";
                    break;
                } else {
                    sp1 = (char*)malloc(tokens[1].length() + 1);
                    strcpy(sp1, tokens[1].c_str());
                    len = atoi(tokens[2].c_str());
                    sp2 = new char [len + 1];

                    int ok = read(&fs, &openTable, sp1, len, sp2);
                    // for (int i = 0; i < len; ++i) {
                    //     std::cerr << sp2[i];
                    // }
                    // std::cerr << "\n";

                    std::string tmp = sp2;
                    std::cout << "Successfuly reading : " << ok << "bytes\n";
                    std::cout << tmp << "\n";
                    free( sp1);
                }
                break;
            case 10:        // exit
                
                fp = fopen(path.c_str(), "wb");
                exitfs(&fs, &openTable, fp);
                tbl_destroy(&openTable);
                return 0;
                break;
            case 11:        // help
                show_help();
                break;
            default:
                std::cout << "没有" << tokens[0] << "这个命令\n";
                break;
        }

    }
    return 0;

}