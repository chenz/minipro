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

typedef int(*device_predicate_t)(const device_t*, const char*);

static void print_device_fields_json(const device_t* device, const char* indent) {
	char chip_id[9];
	assert(device->chip_id_bytes_count <= 4);
	snprintf(chip_id, sizeof(chip_id), "%08x", device->chip_id);
	#define PRINT(fmt, args...) printf("%s" fmt "\n", indent, ##args)
	PRINT("\"name\": \"%s\",", device->name);
	PRINT("\"protocol_id\": %u,", device->protocol_id);
	PRINT("\"variant\": %u,", device->variant);
	PRINT("\"read_buffer_size\": %u,", device->read_buffer_size);
	PRINT("\"write_buffer_size\": %u,", device->write_buffer_size);
	PRINT("\"code_memory_size\": %u,", device->code_memory_size);
	PRINT("\"data_memory_size\": %u,", device->data_memory_size);
	PRINT("\"data_memory2_size\": %u,", device->data_memory2_size);
	PRINT("\"chip_id\": \"%s\",", chip_id + 8 - device->chip_id_bytes_count * 2);
	PRINT("\"chip_id_bytes_count\": %u,", device->chip_id_bytes_count);
	PRINT("\"opts1\": 0x%x,", device->opts1);
	PRINT("\"opts2\": 0x%x,", device->opts2);
	PRINT("\"opts3\": 0x%x,", device->opts3);
	PRINT("\"opts4\": 0x%x,", device->opts4);
	PRINT("\"package_details\": 0x%x,", device->package_details);
	PRINT("\"write_unlock\": 0x%x", device->write_unlock);
	#undef PRINT
}

void print_device_json(const device_t* device, const char* indent, const int last) {
	char field_indent[100] = {0};
	strcat(field_indent, indent);
	strcat(field_indent, "  ");
	printf("%s{\n", indent);
	print_device_fields_json(device, field_indent);
	printf("%s}%s\n", indent, last ? "" : ",");
}

void print_devices_json(const device_predicate_t pred, const char* arg, const int single) {
	if(!single) {
		printf("[\n");
	}
	const device_t* prev_device = NULL;
	for(const device_t* device = devices; device->name; device++) {
		if(pred(device, arg)) {
			if(prev_device) {
				print_device_json(prev_device, single ? "" : "  ", 0);
			}
			prev_device = device;
		}
	}
	if(prev_device) {
		print_device_json(prev_device, single ? "" : "  ", 1);
	}
	if(!single) {
		printf("]\n");
	}
}

static void print_device_info(const device_t *device) {
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

static void print_device_infos(const device_predicate_t pred, const char* arg, const int single)
{
	int first = 1;
	for(const device_t* device = devices; device->name; device++) {
		if(pred(device, arg)) {
			if(!first) {
				printf("--\n");
			}
			print_device_info(device);
			first = 0;
		}
	}
}

static int all_p(const device_t* dev, const char* arg)
{
	return 1;
}

static int name_equals_p(const device_t* dev, const char* arg)
{
	return strcmp(dev->name, arg) == 0;
}

static int name_contains_p(const device_t* dev, const char* arg)
{
	return strstr(dev->name, arg) != NULL;
}

static void print_help_and_exit(const char *progname, int rv) {
	fprintf(rv ? stderr : stdout,
			"Usage: %s [-s <name>][-n <name>][-c <term>][-j][-h]\n"
			"    -s <name>   List part names that begin with <name>\n"
			"    -n <name>   Output record with name <name>\n"
			"    -c <term>   Output records with name containing <term>\n"
			"    -a          Output all records\n"
			"    -j          Output database record in JSON format\n"
			"                Does not work with -s\n"
			"    -h          Print help and quit (this text)\n",
			progname);
	exit(rv);
}

int main(int argc, char **argv) {
	int search = 0;
	device_predicate_t pred = NULL;
	const char* term = NULL;
	int json = 0;
	int c;
	while((c = getopt(argc, argv, "s:n:c:ajh")) != -1) {
		if(c == 's') {
			search = 1;
			pred = 0;
			term = optarg;
		}
		else if(c == 'n') {
			pred = name_equals_p;
			term = optarg;
		}
		else if(c == 'c') {
			pred = name_contains_p;
			term = optarg;
		}
		else if(c == 'a') {
			pred = all_p;
			term = NULL;
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

	if(!pred && !term && optind + 1 == argc) {
		pred = name_equals_p;
		term = argv[optind];
	}
	else if(optind != argc) {
		print_help_and_exit(argv[0], -1);
	}

	if(pred) {
		if(json) {
			print_devices_json(pred, term, pred == name_equals_p);
		}
		else {
			print_device_infos(pred, term, pred == name_equals_p);
		}
	}
	else if(search) {
		size_t term_len = strlen(term);
		device_t *device;
		for(device = &(devices[0]); device[0].name; device = &(device[1])) {
			if(!strncasecmp(device[0].name, term, term_len)) {
				printf("%s\n", device[0].name);
			}
		}
	}
	else {
		print_help_and_exit(argv[0], -1);
	}

	return 0;
}
