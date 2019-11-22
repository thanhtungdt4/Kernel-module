/*Simple Web server to control movement
 * 
 * Author: Nguyen Thanh Tung(Thanhtungdt4.haui@gmail.com)
**/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/wait.h>

#include "motor_control.h"

#define HTML_FILE		"/home/pi/web-server/index.html"
#define SAVETY_SHUTDOWN		"sudo shutdown -h now"
#define SPEED_FILE		"speed.txt"

int speed_array[5] = {1500000, 2500000, 3500000, 4500000, 5500000};
FILE *fp;

void speed_up()
{
	int speed_index;
	int i;
	char temp;
	//FILE *fp;

	fp = fopen(SPEED_FILE, "r+");
	if (fp == NULL) {
		printf("Can not open file\n");
	}

	fseek(fp, 0, SEEK_SET);
	fread(&temp, sizeof(char), sizeof(temp), fp);
	printf("temp %c\n", temp);
	speed_index = atoi(&temp);
	speed_index++;
	if (speed_index >= 5)
		speed_index = 4;
	printf("speed %d\n", speed_index);
	sprintf(&temp, "%d", speed_index);
	fseek(fp, 0, SEEK_SET);
	fwrite(&temp, sizeof(char), sizeof(temp), fp);

	i = speed_index;
	set_duty_cycle(1, speed_array[i]);
	set_duty_cycle(2, speed_array[i]);
	printf("index: %d speed arr: %d\n",i, speed_array[i]);

	fclose(fp);
}

void speed_down()
{
	int speed_index;
	char temp;
	int i;
	//FILE *fp;

	fp = fopen(SPEED_FILE, "r+");
	if (fp == NULL) {
		printf("Can not open file\n");
	}

	fseek(fp, 0, SEEK_SET);
	fread(&temp, sizeof(char), sizeof(temp), fp);
	printf("temp %c\n", temp);
	speed_index = atoi(&temp);
	speed_index--;
	if (speed_index < 0)
		speed_index = 0;
	printf("speed %d\n", speed_index);
	sprintf(&temp, "%d", speed_index);
	fseek(fp, 0, SEEK_SET);
	fwrite(&temp, sizeof(char), sizeof(temp), fp);

	i = speed_index;
	set_duty_cycle(1, speed_array[i]);
	set_duty_cycle(2, speed_array[i]);
	printf("index: %d speed %d\n",i, speed_array[i]);

	fclose(fp);
}

void board_shutdown()
{
	system(SAVETY_SHUTDOWN);
}

void left_up()
{
	stop_motor(2);
	go_straight(1);
}

void Go_straight()
{
	go_straight(1);
	go_straight(2);
}

void right_up()
{
	stop_motor(1);
	go_straight(2);
}

void oto_stop()
{
	stop_motor(1);
	stop_motor(2);
}

void left_back()
{
	stop_motor(2);
	go_back(1);
}

void Go_back()
{
	go_back(1);
	go_back(2);
}

void right_back()
{
	stop_motor(1);
	go_back(2);
}

int main()
{
	struct sockaddr_in server_addr, client_addr;
	socklen_t sin_len = sizeof(client_addr);
	int fd_server, fd_client;
	char buf[4096];
	int on =1;
	int fd_html;
	char receive[2048];

	fd_server = socket(AF_INET, SOCK_STREAM,0);
	if (fd_server < 0) {
		perror("socket\n");
		exit(1);
	}

	setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(8888);

	if (bind(fd_server, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		perror("bind\n");
		close(fd_server);
		exit(1);
	}

	if (listen(fd_server, 10) == -1) {
		perror("listen\n");
		close(fd_server);
		exit(1);
	}

	/*stop as default*/
	oto_stop();
	/* Set duty cycle default for two pwm devices */
	set_duty_cycle(1, 2000000);
	set_duty_cycle(2, 2000000);

	/*Enable two pwm devices as default*/
	enable_pwm_device(1);
	enable_pwm_device(2);

	/*create speed file to communication among process */
	char temp = '0';

	fp = fopen(SPEED_FILE, "w+");
	if (fp == NULL) {
		printf("create or open file failed\n");
	}
	fwrite(&temp, sizeof(char), sizeof(temp), fp);
	fclose(fp);

	while (1) {
		fd_client = accept(fd_server, (struct sockaddr*)&client_addr, &sin_len);
		if (fd_client == -1) {
			perror("Connection Failed\n");
			continue;
		}

		printf("Get client connection ....\n");

		if (fork() == 0) {
			fd_html = open(HTML_FILE, O_RDWR);
			if (fd_html < 0) {
				perror("Can not open file\n");
				exit(1);
			}
			memset(buf, 0, 4096);
			memset(receive, 0, 2048);
			read(fd_html, buf, 4096);
			write(fd_client, buf, sizeof(buf));

			read(fd_client, receive, sizeof(receive));
			printf("Tung %s\n", receive);

			close(fd_client);
			close(fd_html);

			if (strstr(receive, "POST /SPEED_UP")) {
				speed_up();
			} else if (strstr(receive, "POST /SPEED_DOWN")) {
				speed_down();
			} else if (strstr(receive, "POST /LEFT_UP")) {
				left_up();
			} else if (strstr(receive, "POST /LEFT_BACK")) {
				left_back();
			} else if (strstr(receive, "POST /GO_STRAIGHT")) {
				Go_straight();
			} else if (strstr(receive, "POST /GOO_BACK")) {
				Go_back();
			} else if (strstr(receive, "POST /RIGHT_UP")) {
				right_up();
			} else if (strstr(receive, "POST /RIGHT_BACK")) {
				right_back();
			} else if (strstr(receive, "POST /MOTOR_STOP")) {
				oto_stop();
			} else if (strstr(receive, "POST /SHUTDOWN")) {
				board_shutdown();
			} 

			printf("closing\n");
			return 0;
		}
		wait(NULL);
		close(fd_client);
	}
	return 0;
}
