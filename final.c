#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <mysql/mysql.h>



int main()
{
        /* File in POSIX-programma bedien je via FILE DESCRIPTOR == int */
        int serv_sock_fd, clie_sock_fd, bind_succeeded, listen_succeeded, numbytes, serv_port = 60003, mariaid, i;
        socklen_t clen;
        struct sockaddr_in server, client;
        float ftemp, mtemp;
        char temp[6], query[512], command[120];
        char request[10], response[10] = "Y", response3[10] = "Q"; /* eigenlijk void *, maar void * is de universele datatype*/

        /* request is om te LEZEN van de client en response is SCHRIJVEN vanaf server.*/

        serv_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_succeeded < 0)
        {
                printf("Content-type: text/html\r\n\n");
                printf("<html><head>FAIL</head><body><h1>fail1</h1></body></html>\n");
        }

        server.sin_family = AF_INET;
        server.sin_port = htons(serv_port); /* host-to-network order!*/
        inet_pton(AF_INET, "192.168.178.231", &(server.sin_addr)); /*inet_addr is deprecated*/

        bind_succeeded = bind(serv_sock_fd, (struct sockaddr *)&server, sizeof(server));
        if (listen_succeeded < 0)
        {
                printf("Content-type: text/html\r\n\n");
                printf("<html><head>FAIL</head><body><h1>fail2</h1></body></html>\n");
                close(serv_sock_fd);
        }
        listen_succeeded = listen(serv_sock_fd, 3); /* splitst proces in 2 */
        if (listen_succeeded < 0)
        {
                printf("Content-type: text/html\r\n\n");
                printf("<html><head>FAIL</head><body><h1>fail3</h1></body></html>\n");
                close(serv_sock_fd);
        }
        strcpy(command, ".//home/pi/arduino-cli/bin/arduino-cli upload -p /dev/ttyACM0 -b arduino:avr:uno -i ~/test/Client/clientx");
        system(command);
        /* In de wacht om te accepteren */
        clen = sizeof(struct sockaddr_in);
        clie_sock_fd = accept(serv_sock_fd, (struct sockaddr *)&client, &clen);
        if (clie_sock_fd >= 0)
        {
                numbytes = read(clie_sock_fd, request, sizeof(request));
                if (numbytes >= 0)
                {
                        if (request[0] == response[0])
                        {
                                /* Write (respond) to client! */
                                numbytes = (int)write(clie_sock_fd, response, strlen(response));
                                if (numbytes >= 0)
                                {
                                        numbytes = read(clie_sock_fd, &temp, sizeof(&temp));
                                        if (numbytes >= 0)
                                        {
                                        ftemp = strtof(temp, NULL);
                                        numbytes = (int)write(clie_sock_fd, response3, strlen(response3));
                                                if (numbytes >= 0)
                                                {
                                                        MYSQL *init = mysql_init(NULL);
                                                        if (init == NULL)
                                                        {
                                                        printf("Content-type: text/html\r\n\n");
                                                        printf("<html><head>FAIL</head><body><h1>fail4</h1></body></html>\n");
                                                        return 1;
                                                        }

                                                        if (mysql_real_connect(init, "localhost", "root", "joe", NULL, 0, NULL, 0)
                                                        == NULL)
                                                        {
                                                        printf("Content-type: text/html\r\n\n");
                                                        printf("<html><head>FAIL</head><body><h1>fail5</h1></body></html>\n");
                                                        mysql_close(init);
                                                        return 1;
                                                        }

                                                        if (mysql_query(init, "CREATE DATABASE IF NOT EXISTS sensors"))
                                                        {
                                                        printf("Content-type: text/html\r\n\n");
                                                        printf("<html><head>FAIL</head><body><h1>fail6</h1></body></html>\n");
                                                        mysql_close(init);
                                                        return 1;
                                                        }
                                                        if (mysql_query(init, "USE sensors"))
                                                        {
                                                        printf("Content-type: text/html\r\n\n");
                                                        printf("<html><head>FAIL</head><body><h1>fail7</h1></body></html>\n");
                                                        mysql_close(init);
                                                        return 1;
                                                        }

                                                        if (mysql_query(init, "CREATE TABLE IF NOT EXISTS dht (id INT(255)
                                                        PRIMARY KEY AUTO_INCREMENT, temperatuur FLOAT(16))"))
                                                        {
                                                        printf("Content-type: text/html\r\n\n");
                                                        printf("<html><head>FAIL</head><body><h1>fail8</h1></body></html>\n");
                                                        mysql_close(init);
                                                        return 1;
                                                        }
                                                        MYSQL_ROW row;
                                                        mysql_query(init, "SELECT MAX(id) FROM dht");
                                                        MYSQL_RES *myres = mysql_store_result(init);
                                                        int totalrows = mysql_num_rows(myres);
                                                        int numfields = mysql_num_fields(myres);
                                                        MYSQL_FIELD *mfield;
                                                        while((row = mysql_fetch_row(myres)))
                                                        {
                                                            for(i = 0; i < numfields; i++)
                                                            {
                                                                char *val = row[i];
                                                                mariaid = atoi(val) + 1;
                                                            }
                                                        }
                                                        snprintf(query, 512, "INSERT INTO dht VALUES ('%d', '%f')", mariaid, ftemp);
                                                        if (mysql_query(init, query))
                                                        {
                                                        printf("Content-type: text/html\r\n\n");
                                                        printf("<html><head>FAIL</head><body><h1>fail9</h1></body></html>\n");
                                                        mysql_close(init);
                                                        return 1;
                                                        }

                                                        mysql_query(init, "SELECT * FROM dht ORDER BY id DESC LIMIT 0, 1");
                                                        MYSQL_RES *mysecres = mysql_store_result(init);
                                                        totalrows = mysql_num_rows(mysecres);
                                                        numfields = mysql_num_fields(mysecres);
                                                        while((row = mysql_fetch_row(mysecres)))
                                                        {
                                                            for(i = 0; i < numfields-1; i++)
                                                            {
                                                                char *val = row[i];
                                                                char *ctemp = row[i+1];
                                                                mariaid = atoi(val);
                                                                mtemp = strtof(ctemp, NULL);
                                                            }
                                                        }
                                                        mysql_library_end();
                                                }
                                        }
                                }
                        }
                }
        }
        close(clie_sock_fd);
        close(serv_sock_fd);
        printf("Content-type: text/html\r\n\n");
        printf("<html><head>DHT sensor</head><body><h1>Temperatuur: %.2f</h1></body></html>\n", mtemp);
        return 0;
}

