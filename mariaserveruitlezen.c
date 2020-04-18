#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h> //Misschien niet
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
        char temp[6], query[512], command[79];
        char request[10], response[10] = "Y", response3[10] = "Q"; /* eigenlijk void *, maar void * is de universele datatype*/
        /* request is om te LEZEN van de client en response is SCHRIJVEN vanaf server.*/
        serv_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (serv_sock_fd < 0)
        {
                printf("Geen socket voor jou!\n");
        }
        else
        {
                printf("Socket aanmaken geslaagd!\n");
        }

        server.sin_family = AF_INET;
        server.sin_port = htons(serv_port); /* host-to-network order!*/
        inet_pton(AF_INET, "192.168.178.231", &(server.sin_addr)); //inet_addr is deprecated

        bind_succeeded = bind(serv_sock_fd, (struct sockaddr *)&server, sizeof(server));
        if (bind_succeeded < 0)
        {
                printf("Bind failed!\n");
                close(serv_sock_fd);
        }
        else
        {
                printf("Socket found!\n");
        }

        listen_succeeded = listen(serv_sock_fd, 3); /* splitst proces in 2 */
        if (listen_succeeded < 0)
        {
                printf("We gaan niet luisteren!\n");
        }
        else
        {
                printf("Listening for incoming connections on port %d...\n", serv_port);
        }

        /* In de wacht om te accepteren */
        printf("Ik maak de arduino even wakker\n");
        strcpy(command, "arduino-cli upload -p /dev/ttyACM0 -b arduino:avr:uno -i ~/test/Client/clientx");
        system(command);
        clen = sizeof(struct sockaddr_in);
        clie_sock_fd = accept(serv_sock_fd, (struct sockaddr *)&client, &clen);
        if (clie_sock_fd < 0)
        {
                printf("Accept failed!\n");
        }
        else
        {
                printf("Connection accepted!\n");
                numbytes = read(clie_sock_fd, request, sizeof(request));
                if (numbytes < 0)
                {
                        printf("No bytes read!\n");
                }
                else
                {
                        printf("The first byte that was received is: %c\n", request[0]);
                        if (request[0] == response[0])
                        {
                                /* Write (respond) to client! */
                                numbytes = (int)write(clie_sock_fd, response, strlen(response));
                                if (numbytes < 0)
                                {
                                        printf("No bytes sent to client!\n");
                                }
                                else
                                {
                                        printf("%d bytes sent to client\n", numbytes);
                                        numbytes = read(clie_sock_fd, &temp, sizeof(&temp));
                                        printf("Temp is ziek veel bytes: %d\n", numbytes);
                                        if (numbytes < 0)
                                        {
                                                printf("No temperature received from client!\n");
                                        }
                                        else
                                        {
                                        printf("String versie:\n");
                                        printf("Temperature: %s\n", temp);
                                        ftemp = strtof(temp, NULL);
                                        printf("Float versie:\n");
                                        printf("Temperature: %.2f\n", ftemp);
                                        numbytes = (int)write(clie_sock_fd, response3, strlen(response3));
                                        printf("Een Q kost: %d\n", numbytes);
                                                if (numbytes < 0)
                                                {
                                                printf("Geen Q gestuurd!\n");
                                                }
                                                else
                                                {
                                                        printf("Handshake is complete\n");
                                                        MYSQL *init = mysql_init(NULL);
                                                        if (init == NULL)
                                                        {
                                                        fprintf(stderr, "%s\n", mysql_error(init));
                                                        return 1;
                                                        }

                                                        if (mysql_real_connect(init, "localhost", "root", "joe", NULL, 0, NULL, 0) == NULL)
                                                        {
                                                        fprintf(stderr, "%s\n", mysql_error(init));
                                                        mysql_close(init);
                                                        return 1;
                                                        }

                                                        if (mysql_query(init, "CREATE DATABASE IF NOT EXISTS sensors"))
                                                        {
                                                        fprintf(stderr, "%s\n", mysql_error(init));
                                                        mysql_close(init);
                                                        return 1;
                                                        }
                                                        printf("Databasie maken lukt mij wel\n");
                                                        if (mysql_query(init, "USE sensors"))
                                                        {
                                                        fprintf(stderr, "%s\n", mysql_error(init));
                                                        mysql_close(init);
                                                        return 1;
                                                        }
                                                        printf("sensors kan ik wel gebruiken\n");

                                                        if (mysql_query(init, "CREATE TABLE IF NOT EXISTS dht (id INT(255) PRIMARY KEY AUTO_INCREMENT, temperatuur FLOAT(16))"))
                                                        {
                                                        fprintf(stderr, "%s\n", mysql_error(init));
                                                        mysql_close(init);
                                                        return 1;
                                                        }
                                                        printf("Tabelletje lukt ook nog wel.\n");
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


                                                        printf("Hier?\n");
                                                        //mariaid = mysql_insert_id(init) + 1;
                                                        //mariaid = 0;
                                                        snprintf(query, 512, "INSERT INTO dht VALUES ('%d', '%f')", mariaid, ftemp);
                                                        if (mysql_query(init, query))
                                                        {
                                                        fprintf(stderr, "%s\n", mysql_error(init));
                                                        mysql_close(init);
                                                        return 1;
                                                        }

                                                        printf("De meest recente temperatuur meting aan het opvragen.\n");
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
                                                        printf("De meest recent gemeten temp is:%.2f \n", mtemp);
                                                        printf("Dit is logisch omdat het ID: %d de hoogste is in de tabel.\n", mariaid);
                                                        printf("OF TOCH HIERRR?\n");
                                                        mysql_library_end();
                                                        printf("Alles ging goed doei.\n");
                                                }
                                        }
                                }
                        }
                        else
                        {
                                printf("Geen 'Y' ontvangen!\n");
                        }
                }
        }
        close(clie_sock_fd);
        close(serv_sock_fd);
        return 0;
}
