#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include "Tea5767_lib.h"

static struct option long_options[] = {
	{"set_freq",     required_argument, 0, 'f'},
	{"find_station", required_argument, 0, 's'},
	{"set_muted",    required_argument, 0, 'm'},
	{"set_standby",  required_argument, 0, 't'},
	{"set_stereo",   required_argument, 0, 'e'},
	{"get_freq",     no_argument,       0, 'a'},
	{"get_ready",    no_argument,       0, 'b'},
	{"is_muted",     no_argument,       0, 'c'},
	{"get_stations", no_argument,       0, 'd'},
	{"is_stereo",    no_argument,       0, 'g'},
	{"next_station", no_argument,       0, 'i'},
	{"help",         no_argument,       0, 'h'},
	{0,              0,                 0,  0 },
};

void usage(char **argv)
{
    printf("usage: %s --help --> help\n"
        "\t\t      --set_freq     <freq_val>   --> set frequency\n"
        "\t\t      --find_station <min level>  --> find stations\n"
        "\t\t      --set_muted    <true/false> --> set muted\n"
        "\t\t      --set_standby  <true/false> --> set standby\n"
        "\t\t      --set_stereo   <true/false> --> set stereo\n"
        "\t\t      --get_freq                  --> get frequency\n"
	"\t\t      --get_ready                 --> get ready\n"
        "\t\t      --is_muted                  --> is muted\n"
        "\t\t      --get_stations              --> get stations number\n"
        "\t\t      --is_stereo                 --> is stereo or not\n"
        "\t\t      --next_station              --> jumpt to next station\n",
	argv[0]);

	exit(0);
}

int main(int argc, char **argv)
{
	int opt, index, ret;
	uint32_t minlvl, get_freq, get_rdy, station_no;
	bool muted, stby, stereo, is_ste, is_mute;
	double freq;

	if (argc < 2) {
		printf("need option\n");
		usage(argv);
	}

	while ((opt = getopt_long(argc, argv, "f:s:m:t:e:abcdghi", long_options,
		&index)) != -1) {

		switch (opt) {
		case 'f':
			freq = atof(optarg);
			printf("frequency: %f\n", freq);
			ret = set_frequency(freq);
			if (ret) {
				printf("Can not set frequency\n");
				return ret;
			}
			break;

		case 's':
			minlvl = atoi(optarg);
			printf("minlvl: %d\n", minlvl);
			find_stations(minlvl);
			break;

		case 'm':
			if (!strcmp(optarg, "true"))
				muted = true;
			else
				muted = false;
			printf("muted: %s\n", muted ? "true" : "false");
			set_muted(muted);
			break;
		case 't':
			if (!strcmp(optarg, "true"))
				stby = true;
			else
				stby = false;
			printf("standby: %s\n", stby ? "true" : "false");
			set_standby(stby);
			break;

		case 'e':
			if (!strcmp(optarg, "true"))
				stereo = true;
			else
				stereo = false;
			printf("Stereo: %s\n", stereo ? "true" : "false");
			set_stereoNC(stereo);
			break;

		case 'a':
			get_freq = get_frequency();
			printf("frequency set is: %d\n", get_freq);
			break;

		case 'b':
			get_rdy = get_ready();
			printf("ready number: %d\n", get_rdy);
			break;

		case 'c':
			is_mute = is_muted();
			printf("is muted: %s\n", is_mute ? "true" : "false");
			break;

		case 'd':
			station_no = get_stations();
			printf("station number: %d\n", station_no);
			break;

		case 'g':
			is_ste = is_stereo();
			printf("is stereo: %s\n", is_ste ? "true" : "false");
			break;

		case 'i':
			printf("jump to next station\n");
			next_stations();
			break;

		case 'h':
			usage(argv);

		default:
			usage(argv);	
		}
	}

	return 0;
}
