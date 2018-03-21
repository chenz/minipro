#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "minipro.h"
#include "byte_utils.h"
#include "database.h"
#include "error.h"

void print_help_and_exit(const char *progname, int rv) {
	fprintf(rv ? stderr : stdout,
			"Usage: %s [-s][-j][-h] <name>\n"
			"    -s    List part names that begin with <name>\n"
			"    -j    Output database record in JSON format\n"
			"    -h    Print help and quit (this text)\n",
			progname);
	exit(rv);
}

void print_device_info_json(device_t *device) {
	char buf[100];
	printf("{\"name\": \"%s\",\n", device->name);
	printf(" \"protocol_id\": %u,\n", device->protocol_id);
	printf(" \"variant\": %u,\n", device->variant);
	printf(" \"addressing_mode\": %u,\n", device->addressing_mode);
	printf(" \"read_buffer_size\": %u,\n", device->read_buffer_size);
	printf(" \"write_buffer_size\": %u,\n", device->write_buffer_size);
	printf(" \"word_size\": %u,\n", device->word_size);
	printf(" \"code_memory_size\": %u,\n", device->code_memory_size);
	printf(" \"data_memory_size\": %u,\n", device->data_memory_size);
	printf(" \"data_memory2_size\": %u,\n", device->data_memory2_size);
	printf(" \"chip_id\": \"");
	snprintf(buf, sizeof(buf), "%08x", device->chip_id);
	assert(device->chip_id_bytes_count <= 4);
	printf("%s\",\n", buf + 8 - device->chip_id_bytes_count * 2);
	printf(" \"chip_id_bytes_count\": %u,\n", device->chip_id_bytes_count);
	printf(" \"opts1\": %u,\n", device->opts1);
	printf(" \"opts2\": %u,\n", device->opts2);
	printf(" \"opts3\": %u,\n", device->opts3);
	printf(" \"opts4\": %u,\n", device->opts4);
	printf(" \"package_details\": %u,\n", device->package_details);
	printf(" \"write_unlock\": %u,\n", device->write_unlock);
	printf(" \"fuses\": %u}\n", device->fuses ? 1 : 0); /* TODO */
}

void print_device_info(device_t *device) {
	printf("Name: %s\n", device->name);

	/* Memory shape */
	printf("Memory: %d", device->code_memory_size / WORD_SIZE(device));
	switch(device->opts4 & 0xFF000000) {
		case 0x00000000:
			printf(" Bytes");
			break;
		case 0x01000000:
			printf(" Words");
			break;
		case 0x02000000:
			printf(" Bits");
			break;
		default:
			ERROR2("Unknown memory shape: 0x%x\n", device->opts4 & 0xFF000000);
	}
	if(device->data_memory_size) {
		printf(" + %d Bytes", device->data_memory_size);
	}
	if(device->data_memory2_size) {
		printf(" + %d Bytes", device->data_memory2_size);
	}
	printf("\n");

	unsigned char package_details[4];
	format_int(package_details, device->package_details, 4, MP_LITTLE_ENDIAN);
	/* Package info */
	printf("Package: ");
	if(package_details[0]) {
		printf("Adapter%03d.JPG\n", package_details[0]);
	} else if(package_details[3]) {
		printf("DIP%d\n", package_details[3] & 0x7F);
	} else {
		printf("ISP only\n");
	}

	/* ISP connection info */
	printf("ISP: ");
	if(package_details[1]) {
		printf("ICP%03d.JPG\n", package_details[1]);
	} else {
		printf("-\n");
	}

	printf("Protocol: 0x%02x\n", device->protocol_id);
	printf("Read buffer size: %d Bytes\n", device->read_buffer_size);
	printf("Write buffer size: %d Bytes\n", device->write_buffer_size);
}

int main(int argc, char **argv) {
	int search = 0;
	int json = 0;
	int c;
	const char* term = NULL;
	while((c = getopt(argc, argv, "sjh")) != -1) {
		if(c == 's') {
			search = 1;
		}
		else if(c == 'j') {
			json = 1;
		}
		else if(c == 'h') {
			print_help_and_exit(argv[0], 0);
		}
		else {
			print_help_and_exit(argv[0], -1);
		}
	}

	if(optind + 1 != argc) {
		print_help_and_exit(argv[0], -1);
	}
	term = argv[optind];

	// Listing all devices that starts with argv[2]
	if(search) {
		size_t term_len = strlen(term);
		device_t *device;
		for(device = &(devices[0]); device[0].name; device = &(device[1])) {
			if(!strncasecmp(device[0].name, term, term_len)) {
				printf("%s\n", device[0].name);
			}
		}
		return(0);
	}

	device_t *device = get_device_by_name(term);
	if(!device) {
		ERROR("Unknown device");
	}

	if(json) {
		print_device_info_json(device);
	}
	else {
		print_device_info(device);
	}
	return(0);
}
