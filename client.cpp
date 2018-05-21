//client.cpp -  TCP(UDP)/IP клиент

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>

#define BUFFSIZE 100 //максимальный размер сообщения, 100 байт

using namespace std;

int main(int argc, char **argv)
{
    struct sockaddr_in sin;         //структура IP-адреса точки назначения
    struct hostent *phe;            //указатель на запись с информацией о хосте
    struct protoent *ppe;           //указатель на запись с информацией о протоколе
    int client_sock_type;           //тип сокета
    int client_sock;                //дескриптор сокета
    int length_msg;                 //длина отправляемого/принимаемого сообщения
    char msg[BUFFSIZE];             //буфер для принимаемого сообщения
    memset(msg, 0, sizeof(msg));    //очищаем буфер приема
    char* dyn_msg;                  //динамический буфер для отправляемого сообщения
    string input;                   //строка для проверки длины ввода

    fd_set fd;                      //структура для работы с дескрипторами
    FD_ZERO( &fd );                 //инициализация нулями

    struct timeval tv;              //структура для времени ожидания
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    if(argc == 4) 	  //проверяем количество переданных аргументов
    {
        const char *host = argv[1];
        const char *port = argv[2];
        const char *transport = argv[3];
        memset(&sin, 0, sizeof(sin));                           //обнуляем структуру адреса
        sin.sin_family = AF_INET;                               //указываем тип адреса (IPv4)
        sin.sin_port = htons((unsigned short)atoi(port));       //задаем порт
        if(phe = gethostbyname(host))                           //преобразовываем имя хоста в IP-адрес
            memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);  //копируем IP-адрес в структуру сервера
        if((ppe = getprotobyname(transport)) == 0)              //преобразовываем имя транспортного протокола в номер протокола
        {
            cout<<"Ошибка преобразования имени транспортного протокола: "<<strerror(errno)<<endl;	//в случае неудачи выводим сообщение ошибки
            return -1;
        }
        if(strcmp(transport, "udp") == 0)             //используем имя протокола для определения типа сокета
            client_sock_type = SOCK_DGRAM;            //для udp
        else
            client_sock_type = SOCK_STREAM;           //для tcp

        while(1)
        {
            cout<<"Введите сообщение(введите 'exit' для завершения программы):"<<endl;
            cin>>input;
            length_msg = input.length()+1;   //длина строки + '\0'
            if( length_msg > BUFFSIZE)       //проверка на длину сообщения
            {
                cout<<"Слишком большая длина сообщения, сообщение должно быть длиной до "<<BUFFSIZE<<" байт"<<endl;
                continue;
            }
            dyn_msg = new char[length_msg];       //выделяем память под сообщение
            strcpy(dyn_msg,input.c_str());        //копируем строку ввода в буфер отправки
            input.erase(0,length_msg);            //очищаем переменную ввода
            if(strncmp(dyn_msg, "exit", 5) == 0)  //если введен "exit" завершаем программу
            {
                cout<<"Завершение программы..."<<endl;
                return 0;
            }

            client_sock = socket(PF_INET, client_sock_type, ppe->p_proto);      //создание сокета
            if(client_sock < 0)
            {
                cout<<"Ошибка создания сокета: "<<strerror(errno)<<endl;        //в случае неудачи выводим сообщение ошибки
                return -1;
            }
            FD_SET( client_sock, &fd );                                         //добавление дескриптора client_sock в структуру

            if(strcmp(transport, "tcp") == 0)                                               //если протокол tcp, то проводим подключение к серверу
            {
                if(connect(client_sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)          //попытка подключить сокет
                {
                    cout<<"Не удалось подключится к серверу: "<<strerror(errno)<<endl;      //в случае неудачи выводим сообщение ошибки
                    close(client_sock);
                    return -1;
                }
                else
                    cout<<"Установленно соединение с "<<host<<":"<<port<<" "<<transport<<endl;
            }

            if(sendto(client_sock, dyn_msg, length_msg, 0, ( struct sockaddr* )&sin,sizeof( struct sockaddr_in )) < 0)		//отсылаем серверу сообщение
            {
                cout<<"Не удалось отправить данные серверу: "<<strerror(errno)<<endl;
                close(client_sock);
                return -1;
            }
            cout<<"Серверу отправлено: "<<dyn_msg<<endl;
            delete dyn_msg;                     //освобождаем память

            if( select( client_sock + 1, &fd, NULL, NULL, &tv ) == 0 )      //ожидание входящих соединений на дескриптор client_sock
            {
                cout<<"Время ожидания истекло"<<endl;                       //если ответ от сервера не приходит в течении 5 секунд - завершаем работу программы
                close(client_sock);
                return -1;

            }
            if(FD_ISSET(client_sock, &fd))                       //если появляется входящее подключение
            {
                if(read(client_sock, &msg, sizeof(msg)) < 0)     //слушаем ответ сервера
                {
                    cout<<"Нет ответа от сервера: "<<strerror(errno)<<endl;
                    close(client_sock);
                    return -1;
                }
                cout<<"От сервера получено: "<<msg<<endl<<endl;
                length_msg = strlen(msg)+1;
                memset(msg, 0, length_msg);     //очищаем буфер приема
            }
            close(client_sock);                 //закрываем сокет
        }
    }
    else                                                                          //если число аргументов не совпадает
        cout<<"Использование: server \"server\" \"port\" \"protocol\""<<endl;     //выводим подсказку по использованию
    return 0;
}

