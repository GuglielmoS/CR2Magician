/*****************************************************************************
 * CR2Magician is a software that allows you to retrieve informations from   *
 * a .cr2 file.                                                              *
 * CR2 is the proprietary raw file format used by Canon for storing the data *
 * taken from a Camera Sensor.                                               *
 * Thanks to the author of the web page http://lclevy.free.fr/cr2/:          *
 *     "Understanding What is stored in a Canon RAW .CR2 file, How and Why"  *
 * I've used it for understanding that format and writing this software :D   *
 *                                                                           *
 * Copyright (C) 2011  Guglielmo Fachini                                     *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation, either version 3 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            * 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LITTLE_ENDIAN
	#define LITTLE_ENDIAN 1234
#endif

#ifndef BIG_ENDIAN
	#define BIG_ENDIAN 4321
#endif

/* UNSIGNED TYPES */
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;

/* SIGNED TYPES */
typedef signed char s8;
typedef signed short int s16;
typedef signed int s32;

/* OTHER TYPES */
typedef char * string;

/* BYTE ORDER TYPE */
typedef signed short int byte_order;

/* BOOLEAN TYPE */
typedef enum bool {false = 0, true} boolean;

/*** FILE's FUNCTIONS DEFAULT PARAMS ***/
#define DEFAULT_FILE_PARAMS FILE* stream

/*** NUMBER OF IFD IN A .CR2 FILE ***/
#define NUMBER_OF_IFD 4

/*** TAGS USED IN IFD, EXIF AND MAKERNOTE SECTIONs ***/
#define CR2_TAG_IMAGE_WIDTH   0x0100
#define CR2_TAG_IMAGE_HEIGHT  0x0101
#define CR2_TAG_COMPRESSION   0x0103
#define CR2_TAG_MODEL         0x0110
#define CR2_TAG_DATE_TIME     0x0132
#define CR2_TAG_EXIF          0x8769
#define CR2_TAG_EXPOSURE_TIME 0x829A
#define CR2_TAG_F_NUMBER      0x829D
#define CR2_TAG_MAKERNOTE     0x927C
#define CR2_TAG_OWNER_NAME    0x0009
#define CR2_TAG_LENS_MODEL    0x0095
#define CR2_TAG_COLOR_SPACE   0x00B4
#define CR2_TAG_FOCAL_LENGTH  0x0002

/*** USEFUL MACROS ***/
#define BYTE_TO_LITTLE_ENDIAN(byte)     ((((byte)  >>  4) & 0x0F)       | (((byte)  << 4) & 0xF0))
#define WORD_TO_LITTLE_ENDIAN(word)     ((((word)  >>  8) & 0x00FF)     | (((word)  << 8) & 0xFF00))
#define DWORD_TO_LITTLE_ENDIAN(dword)   ((((dword) >> 24) & 0x000000FF) | (((dword) >> 8) & 0x0000FF00) | \
   									     (((dword) <<  8) & 0x00FF0000) | (((dword) << 24) & 0xFF000000))
#define DOUBLE_TO_LITTLE_ENDIAN(d_data) (double)__builtin_bswap64((int64_t)(d_data))
#define IS_BIG_ENDIAN (FILE_BYTE_ORDER == BIG_ENDIAN)
#define IS_LITTLE_ENDIAN (FILE_BYTE_ORDER == LITTLE_ENDIAN)

/**
 * CR2_Header
 * It contains all the information about the image's header.
 */
typedef struct {
  u16 file_byte_order;          /* 0x4949 -> "Little Endian" o 0x4d4d-> "Big Endian" */
  s16 TIFF_magic_word;   		/* 0x002a */
  s32 TIFF_offset;       		/* 0x00000010 */
  s16 CR2_magic_word;    		/* "CR" -> 0x5243 */
  s8  CR2_major_version; 		/* 2 */ 
  s8  CR2_minor_version; 		/* 0 */
  u32 RAW_IFD_offset;    		/* */
} CR2_Header;

/**
 * CR2_IFD_Directory_Entry
 * Represents the structure of an IFD directory entry.
 * Format:
 *   1. TAG ID
 *   2. TAG TYPE
 *   3. NUMBER OF VALUE
 *   4. VALUE, OR POINTER TO THE DATA
 */
typedef struct {
	u16 tag_ID;
	u16 tag_type;
	u32 number_of_value;
	u32 value;
} CR2_IFD_Directory_Entry;

/**
 * CR2_IDF
 * Represents an Image File Directory section.
 * It's composed of various Directory Entry, so it has
 * a pointer to the first element of the array that contains 
 * directory entries, and a unsigned short that represents 
 * the size of it.
 * next_IFD_offset is used for identifing the next IFD section.
 * If it is equal to 0, it means that it is the last IFD section.
 */
typedef struct {
	CR2_IFD_Directory_Entry *dir_entries;
	u16 dir_entries_length;
	u32 next_IFD_offset;
} CR2_IFD;


/**
 * CR2_Image_Info
 * It contains all the information related to the picture.
 */
typedef struct {	
	string exposure_time;
	string color_space;
	string owner_name;
	string lens_model;
	string date_time;
	string f_number;
	string model;
	u16 image_height;
	u16 focal_length;
	u16 image_width;
	u16 compression;
} CR2_Image_Info;


/***************************************
 * Prototypes functions                *
 ***************************************/

/*** I/O FUNCTIONS ***/
string  get_string(DEFAULT_FILE_PARAMS);
u16 	get_ushort(DEFAULT_FILE_PARAMS);
u32 	get_uint(DEFAULT_FILE_PARAMS);
u32* 	get_urational(DEFAULT_FILE_PARAMS);
s16 	get_sshort(DEFAULT_FILE_PARAMS);
s32 	get_sint(DEFAULT_FILE_PARAMS);
float   get_float(DEFAULT_FILE_PARAMS);
double  get_double(DEFAULT_FILE_PARAMS);

/*** CR2 FUNCTIONS ***/
byte_order CR2_determine_byte_order(u16 raw_byte_order);
boolean    CR2_get_header(FILE * stream, CR2_Header * buffer);
boolean    CR2_print_header(FILE * stream, CR2_Header * header);
u32        CR2_get_IFD(FILE * stream, CR2_IFD * ifd, u32 offset);
boolean    CR2_destroy_IFD(CR2_IFD *ifd);
boolean    CR2_print_IFD(FILE * stream, CR2_IFD * ifd, int IFD_id);
boolean    CR2_get_image_info(FILE * stream, CR2_IFD * ifd, CR2_Image_Info * buffer);
boolean    CR2_print_image_info(FILE * stream, CR2_Image_Info * info);


/***************************************
 * GLOBAL VARIABLES
 ***************************************/
byte_order FILE_BYTE_ORDER;

/***************************************
 * ENTRY POINT.
 ***************************************/
int main(int argc, char *argv[]) {
	CR2_Header *header = (CR2_Header*)malloc(sizeof(CR2_Header));
	CR2_IFD *ifds[NUMBER_OF_IFD];
	CR2_Image_Info *image_info = (CR2_Image_Info*)malloc(sizeof(CR2_Image_Info));

	FILE *file = fopen("tmp.CR2", "rb");
	
	u32 ifd_offset;
	u32 i;
		
	for (i = 0; i < NUMBER_OF_IFD; i++) {
		ifds[i] = (CR2_IFD*)malloc(sizeof(CR2_IFD));
	}
	
	if (CR2_get_header(file, header)) {
		CR2_print_header(stdout, header);
	}
	else {
		fprintf(stderr, "NOTHING TO DO...\n");
		exit(EXIT_FAILURE);
	}
	
	ifd_offset = ftell(file);
	for (i = 0; i < NUMBER_OF_IFD; i++) {
		if (CR2_get_IFD(file, ifds[i], ifd_offset)) {
			CR2_print_IFD(stdout, ifds[i], i);
		}
		else {
			fprintf(stderr, "NOTHING TO DO...\n");
			exit(EXIT_FAILURE);
		}
		ifd_offset = ifds[i]->next_IFD_offset;
	}
	
	if (CR2_get_image_info(file, ifds[0], image_info)) {
		CR2_print_image_info(stdout, image_info);
	}
	else {
		fprintf(stderr, "NOTHING TO DO...\n");
		exit(EXIT_FAILURE);
	}
	
	exit(EXIT_SUCCESS);
}

/**
 * get_string
 * Params:
 *  1. FILE stream used for retriving data
 * Return:
 * 	A byte buffer that contains a string with the terminator
 * character ('\0' or 0x00).
 * It returns NULL if something goes wrong.
 */
string get_string(DEFAULT_FILE_PARAMS) {
	char tmp_string[BUFSIZ] = {0};
	char *final_string;
	char ch;
	int  i;
	
	if (stream == NULL) {
		return NULL;
	}
	
	i = 0;
	ch = 0x41;
	memset(tmp_string, 0x00, BUFSIZ);
	
	while (ch != 0x00) {
    	if (fread((char*)&ch, sizeof(char), 1, stream) != 1) {
			perror("[ERROR-get-string-fread]");
			return NULL;
		}
		
		if (IS_BIG_ENDIAN) {
			ch = BYTE_TO_LITTLE_ENDIAN(ch);
		}
    	
		tmp_string[i++] = ch;
  	}
	final_string = strdup(tmp_string);
	
	return final_string;
}

/**
 * get_ushort
 * Params:
 *  1. FILE stream used for retriving data
 * Return:
 * 	An unsigned short variable (16 bit) that contains the next
 *  two unsigned byte of data.
 */
u16 get_ushort(DEFAULT_FILE_PARAMS) {
	u16 raw_data;
	
	if (stream != NULL) {
		if (fread((u16*)&raw_data, sizeof(u16), 1, stream) != 1) {
			perror("[ERROR-fread]");
		}
		else {
			if (IS_BIG_ENDIAN) {
				raw_data = WORD_TO_LITTLE_ENDIAN(raw_data);
			}
		}
	}
	
	return raw_data;
}

/**
 * get_uint
 * Params:
 *  1. FILE stream used for retriving data
 * Return:
 * 	An unsigned int variable (32 bit) that contains the next
 *  four byte of data.
 */
u32 get_uint(DEFAULT_FILE_PARAMS) {
	u32 raw_data;
	
	if (stream != NULL) {
		if (fread((u32*)&raw_data, sizeof(u32), 1, stream) != 1) {
			perror("[ERROR-fread]");
		}
		else {
			if (IS_BIG_ENDIAN) {
				raw_data = DWORD_TO_LITTLE_ENDIAN(raw_data);
			}
		}
	}
	
	return raw_data;
}

/**
 * get_srational
 * Params:
 *  1. FILE stream used for retriving data
 * Return:
 *  An unsigned rational, is composed of two unsigned int (32 bit).
 *  So it returns an array of unsigned int with two elements.
 *  If something goes wrong, it returns NULL.
 */
u32* get_urational(DEFAULT_FILE_PARAMS) {
	u32 * raw_buffer;
	
	raw_buffer = NULL;
	if (stream != NULL) {
		raw_buffer = (u32*)malloc(sizeof(u32)*2);
		if (fread((u32*)raw_buffer, sizeof(u32), 2, stream) != 2) {
			perror("[ERROR-fread]");
		}
		else {
			if (IS_BIG_ENDIAN) {
				raw_buffer[0] = DWORD_TO_LITTLE_ENDIAN(raw_buffer[0]);
				raw_buffer[1] = DWORD_TO_LITTLE_ENDIAN(raw_buffer[1]);
			}
		}
	}
	
	return raw_buffer;
}

/**
 * get_schar
 * Params:
 *  1. FILE stream used for retriving data
 * Return:
 * 	A signed char variable (8 bit) that contains the next byte of data.
 */
s8 get_schar(DEFAULT_FILE_PARAMS) {
	s8 raw_data;
	
	if (stream != NULL) {
		if (fread((s8*)&raw_data, sizeof(s8), 1, stream) != 1) {
			perror("[ERROR-fread]");
		}
		else {
			if (IS_BIG_ENDIAN) {
				raw_data = BYTE_TO_LITTLE_ENDIAN(raw_data);
			}
		}
	}
	
	return raw_data;
}

/**
 * get_sshort
 * Params:
 *  1. FILE stream used for retriving data
 * Return:
 * 	A signed short variable (16 bit) that contains the next
 *  two signed byte of data.
 */  
s16 get_sshort(DEFAULT_FILE_PARAMS) {
	s16 raw_data;
	
	if (stream != NULL) {
		if (fread((s16*)&raw_data, sizeof(s16), 1, stream) != 1) {
			perror("[ERROR-fread]");
		}
		else {
			if (IS_BIG_ENDIAN) {
				raw_data = WORD_TO_LITTLE_ENDIAN(raw_data);
			}
		}
	}
	
	return raw_data;
}

/**
 * get_sint
 * Params:
 *  1. FILE stream used for retriving data
 * Return:
 * 	A signed int variable (32 bit) that contains the next
 *  four byte of data.
 */
s32 get_sint(DEFAULT_FILE_PARAMS) {
	s32 raw_data;
	
	if (stream != NULL) {
		if (fread((s32*)&raw_data, sizeof(s32), 1, stream) != 1) {
			perror("[ERROR-fread]");
		}
		else {
			if (IS_BIG_ENDIAN) {
				raw_data = DWORD_TO_LITTLE_ENDIAN(raw_data);
			}
		}
	}
	
	return raw_data;
}

/**
 * get_float
 * Params:
 *  1. FILE stream used for retriving data
 * Return:
 *  A float number, 4 bytes, in the IEEE format.
 *  It returns 4294967296.000000 if something goes wrong.
 */
float get_float(DEFAULT_FILE_PARAMS) {
	float raw_data;
	
	raw_data = 0xFFFFFFFF; /* 4294967296.000000 */
	if (stream != NULL) {
		if (fread((float*)&raw_data, sizeof(float), 1, stream) != 1) {
			perror("[ERROR-fread]");
		}
		else {
			if (IS_BIG_ENDIAN) {
				raw_data = DWORD_TO_LITTLE_ENDIAN((int)raw_data);
			}
		}
	}
	
	return raw_data;
}

/**
 * get_double
 * Params:
 *  1. FILE stream used for retriving data
 * Return:
 *  A double number, 8 bytes, in the IEEE format.
 *  It returns 18446744073709551616.000000 if something goes wrong.
 */
double get_double(DEFAULT_FILE_PARAMS) {
	double raw_data;
	
	raw_data = 0xFFFFFFFFFFFFFFFF; /* 4294967296.000000 */
	if (stream != NULL) {
		if (fread((double*)&raw_data, sizeof(double), 1, stream) != 1) {
			perror("[ERROR-fread]");
		}
		else {
			if (IS_BIG_ENDIAN) {
				raw_data = DOUBLE_TO_LITTLE_ENDIAN(raw_data);
			}
		}
	}
	
	return raw_data;	
}

/**
 * CR2_get_header
 * Params:
 *  1. FILE stream used for retriving data
 *  2. buffer that will contain the header
 *
 * It gets from the stream the header information, 
 * and stores them into the buffer passed via parameter.
 * If something goes wrong, it prints an error and returns false.
 */
boolean CR2_get_header(FILE * stream, CR2_Header * buffer) {
	boolean no_errors = true;
		
	if (stream != NULL && buffer != NULL) {
		/* move to the beginning of the file */
		if (fseek(stream, 0, SEEK_SET) != 0) {
			perror("[ERROR-fseek]");
		}
		
		/* reading the raw byte order */
		if (fread((u16*)&buffer->file_byte_order, sizeof(u16), 1, stream) != 1) {
			perror("[ERROR]-fread");
			return false;
		}
		
		/* determine the byte order */
		if (CR2_determine_byte_order(buffer->file_byte_order) == -1) {
			fprintf(stderr, "[ERROR-CR2_determine_byte_order] Cannot determine the byte order!\n");
			return false;
		}
		
		/* read the rest of the header */
		buffer->TIFF_magic_word = get_sshort(stream);
		buffer->TIFF_offset = get_sint(stream);
		buffer->CR2_magic_word = get_sshort(stream);
		buffer->CR2_major_version = get_schar(stream);
		buffer->CR2_minor_version = get_schar(stream);
		buffer->RAW_IFD_offset = get_sint(stream);
	}

	return (no_errors == true);
}

/**
 * CR2_determine_byte_order
 * Params:
 *  1. unsigned short that contains the raw rapresentation
 *
 * It determines the byte order of the file,
 * by looking at the CR2_Header raw rapresentation.
 * It returns -1 if something goes wrong, or the 
 * CR2_Header doesn't contain a valid byte order value.
 */
byte_order CR2_determine_byte_order(u16 raw_byte_order) {
	byte_order final_byte_order;
	
	switch (raw_byte_order) {
		/* Little endian */
		case 0x4949:
			final_byte_order = LITTLE_ENDIAN;
			FILE_BYTE_ORDER = LITTLE_ENDIAN;
			break;
		/* Big endian */
		case 0x4D4D:
			final_byte_order = BIG_ENDIAN;
			FILE_BYTE_ORDER = BIG_ENDIAN;
			break;
	
		/* error */
		default:
			fprintf(stderr, "0x%X is not a valid byte order!\n", raw_byte_order);
			final_byte_order = -1;
	}
	
	return final_byte_order;
}

/**
 * CR2_print_header
 * Params:
 *   1. the stream that you want to use for the output (i.e. stdout)
 *   2. the header to print
 *
 * It puts on the stream a description of the header.
 * The format used is:
 *   [CR2_Header]
 *     BYTE ORDER: LITTLE ENDIAN or BIG ENDIAN
 *     TIFF MAGIC WORD: 0x%X
 *     TIFF OFFSET: 0x%X
 *     CR2 MAGIC WORD: 0x%X - %c%c
 *     CR2 MAJOR VERSION: %d
 *     CR2 MINOR VERSION: %d
 *     RAW IFD OFFSET: 0x%X
 *  [/CR2_Header]
 */
boolean CR2_print_header(FILE * stream, CR2_Header * header) {
	if (stream != NULL && header != NULL) {
		fprintf(stream, "[CR2_Header]\n");
		fprintf(stream, "\tBYTE ORDER: %s\n", (IS_LITTLE_ENDIAN) ? "Little Endian" : "Big Endian");
		fprintf(stream, "\tTIFF MAGIC WORD: 0x%X\n", header->TIFF_magic_word);
		fprintf(stream, "\tTIFF OFFSET: 0x%X\n", (u16)header->TIFF_offset);
		fprintf(stream, "\tCR2 MAGIC WORD: 0x%X - %c%c\n", header->CR2_magic_word, 
														   header->CR2_magic_word & 0x00FF, 
														   (header->CR2_magic_word >> 8) & 0x00FF);
		fprintf(stream, "\tCR2 MAJOR VERSION: %d\n", header->CR2_major_version);
		fprintf(stream, "\tCR2 MINOR VERSION: %d\n", header->CR2_minor_version);
		fprintf(stream, "\tRAW IFD OFFSET: 0x%X\n", (u16)header->RAW_IFD_offset);
		fprintf(stream, "[/CR2_Header]\n");
		
		return true;
	}
	
	return false;
}

/**
 * CR2_get_IFD
 * Params:
 *   1. the stream used for reading data
 *   2. the buffer used for storing the IFD section.
 *      it must be allocated after the calling of 
 *      this function.
 *   3. the offset of the IFD section to read
 * Return:
 *   The number of directory entry read.
 */
u32 CR2_get_IFD(FILE * stream, CR2_IFD * ifd, u32 offset) {
	if (stream != NULL && ifd != NULL) {
		u16 dir_entries_length;
		u32 i;
		
		/* move to the IFD offset */
		if (fseek(stream, offset, SEEK_SET) != 0) {
			perror("[ERROR-fseek]");
			return 0;
		}
		
		/* first, read the number of directory entries */
		dir_entries_length = get_ushort(stream);
		if (dir_entries_length <= 0) {
			fprintf(stderr, "[ERROR] Invalid number of directory entries - IFD_OFFSET=0x%X\n", offset);
			return 0;
		}
		
		/* allocate the CR2_IFD Directory Entries array */
		ifd->dir_entries_length = dir_entries_length;
		ifd->dir_entries = (CR2_IFD_Directory_Entry*)malloc(sizeof(CR2_IFD_Directory_Entry)*ifd->dir_entries_length);
		
		/* getting all directory entries from the stream */
		for (i = 0; i < ifd->dir_entries_length; i++) {
			ifd->dir_entries[i].tag_ID = get_ushort(stream);
			ifd->dir_entries[i].tag_type = get_ushort(stream);
			ifd->dir_entries[i].number_of_value = get_uint(stream);
			ifd->dir_entries[i].value = get_uint(stream);
		}
		
		/* getting the next IFD offset */
		ifd->next_IFD_offset = get_uint(stream);
		
		return ifd->dir_entries_length;
	}
	
	return 0;
}

/**
 * CR2_destroy_IFD
 * It free the memory allocated for the ifd.
 */
boolean CR2_destroy_IFD(CR2_IFD *ifd) {
	if (ifd != NULL) {
		free(ifd->dir_entries);
		free(ifd);
		
		return true;
	}
	
	return false;
}

/**
 * CR2_print_ifd
 * Params:
 *   1. the stream that you want to use for the output (i.e. stdout)
 *   2. the ifd section that you want to print
 *   3. the IFD's identifier (i.e. ifd#0 has IDF_id = 0) 
 *
 * It puts on the stream a description of the ifd section choosen.
 * The format used is:
 *   [IFD]
 *     NUMBER OF ENTRY: %d
 *     [ENTRY#0]
 *       TAG ID: 0x%X 
 *       TAG TYPE: %d
 *       NUMBER OF VALUE: %d
 *       VALUE: 0x%X
 *     [/ENTRY#0]
 *     .....
 *     [ENTRY#N]
 *       .....
 *     [/ENTRY#N]
 *     NEXT IFD OFFSET: 0x%X
 *  [/IFD]
 */
boolean CR2_print_IFD(FILE * stream, CR2_IFD * ifd, int IFD_id) {
	if (stream != NULL && ifd != NULL) {
		u32 i;
		
		fprintf(stream, "[IFD#%d]\n", IFD_id);
		for (i = 0; i < ifd->dir_entries_length; i++) {
			fprintf(stream, "\t[ENTRY#%d]\n", i);
			fprintf(stream, "\t\tTAG ID: 0x%X\n", ifd->dir_entries[i].tag_ID);
			fprintf(stream, "\t\tTAG TYPE: %d\n", ifd->dir_entries[i].tag_type);
			fprintf(stream, "\t\tNUMBER OF VALUE: %d\n", ifd->dir_entries[i].number_of_value);
			fprintf(stream, "\t\tVALUE: 0x%X\n", ifd->dir_entries[i].value);
			fprintf(stream, "\t[/ENTRY#%d]\n", i);
		}
		fprintf(stream, "\tNEXT IFD OFFSET: 0x%X\n", ifd->next_IFD_offset);
		fprintf(stream, "[/IFD#%d]\n", IFD_id);
		
		return true;
	}
	
	return false;
}

/**
 * CR2_get_image_info
 * Params:
 *   1. the stream of the .cr2 file
 *   1. the ifd section from which you get information
 *   2. the buffer used for storing information 
 */
boolean CR2_get_image_info(FILE * stream, CR2_IFD * ifd, CR2_Image_Info * buffer) {
	if (stream != NULL && ifd != NULL && buffer != NULL) {
		char tmp_string[BUFSIZ] = {0};
		CR2_IFD * tmp_IFD;
		u32 * values;
		u32 i;
		
		for (i = 0; i < ifd->dir_entries_length; i++) {
			memset(tmp_string, 0x00, BUFSIZ);
			switch (ifd->dir_entries[i].tag_ID) {
								
				case CR2_TAG_OWNER_NAME:
					fseek(stream, ifd->dir_entries[i].value, SEEK_SET);
					buffer->owner_name = get_string(stream);
				break;
								
				case CR2_TAG_LENS_MODEL:
					fseek(stream, ifd->dir_entries[i].value, SEEK_SET);
					buffer->lens_model = get_string(stream);
				break;
				
				case CR2_TAG_MODEL:
					fseek(stream, ifd->dir_entries[i].value, SEEK_SET);
					buffer->model = get_string(stream);
				break;				
								
				case CR2_TAG_IMAGE_WIDTH:
					buffer->image_width = ifd->dir_entries[i].value;
				break;
				
				case CR2_TAG_IMAGE_HEIGHT:
					buffer->image_height = ifd->dir_entries[i].value;
				break;
				
				case CR2_TAG_COMPRESSION:
					buffer->compression = ifd->dir_entries[i].value;
				break;
								
				case CR2_TAG_DATE_TIME:
					fseek(stream, ifd->dir_entries[i].value, SEEK_SET);
					buffer->date_time = get_string(stream);
				break;
				
				case CR2_TAG_EXIF:
				case CR2_TAG_MAKERNOTE:
					tmp_IFD = (CR2_IFD*)malloc(sizeof(CR2_IFD));
					if (CR2_get_IFD(stream, tmp_IFD, ifd->dir_entries[i].value) == 0) {
						fprintf(stderr, "[ERROR-CR2_get_image_info]\n");
						return false;
					}
					/* recursive */
					if (!CR2_get_image_info(stream, tmp_IFD, buffer)) {
						fprintf(stderr, "[ERROR-CR2_get_image_info]\n");
						CR2_destroy_IFD(tmp_IFD);
						return false;
					}
					/* free memory */
					CR2_destroy_IFD(tmp_IFD);
				break;

				case CR2_TAG_FOCAL_LENGTH:
					fseek(stream, ifd->dir_entries[i].value+2, SEEK_SET);
					buffer->focal_length = get_ushort(stream);
				break;
								
				case CR2_TAG_EXPOSURE_TIME:
					fseek(stream, ifd->dir_entries[i].value, SEEK_SET);
					values = get_urational(stream);
					sprintf(tmp_string, "%d/%ds", values[0], values[1]);
					buffer->exposure_time = (char*)malloc(strlen(tmp_string)*sizeof(char));
					strcpy(buffer->exposure_time, tmp_string);
				break;
				
				case CR2_TAG_F_NUMBER:
					fseek(stream, ifd->dir_entries[i].value, SEEK_SET);
					values = get_urational(stream);
					sprintf(tmp_string, "f/%.1f", (float)values[0]/values[1]);
					buffer->f_number = (char*)malloc(strlen(tmp_string)*sizeof(char));
					strcpy(buffer->f_number, tmp_string);
				break;
								
				case CR2_TAG_COLOR_SPACE:
					fseek(stream, ifd->dir_entries[i].value, SEEK_SET);
					buffer->color_space = (get_ushort(stream) == 1) ? "sRGB" : "Adobe RGB";
				break;
			}
		}
		
		return true;
	}
	
	return false;
}

/**
 * CR2_print_image_info
 * Params:
 *   1. the stream where you want to print the information
 *   2. the variable that contains the information to print
 * Return:
 *   False if something goes wrong, true instead.
 */
boolean CR2_print_image_info(FILE * stream, CR2_Image_Info * info) {
	if (stream != NULL && info != NULL) {
		fprintf(stream, "[Image_Info]\n");
		fprintf(stream, "\tCamera model:  %s\n", info->model);
		fprintf(stream, "\tLens model:    %s\n", info->lens_model);
		fprintf(stream, "\tOwner name:    %s\n", info->owner_name);
		fprintf(stream, "\tShot's date:   %s\n", info->date_time);
		fprintf(stream, "\tImage width:   %d\n", info->image_width);
		fprintf(stream, "\tImage hight:   %d\n", info->image_height);
		fprintf(stream, "\tColor Space:   %s\n", info->color_space);
		fprintf(stream, "\tCompression:   %s\n", (info->compression == 6) ? "Old jpeg" : "Unknown");
		fprintf(stream, "\tExposure Time: %s\n", info->exposure_time);
		fprintf(stream, "\tF Number:      %s\n", info->f_number);
		fprintf(stream, "\tFocal Length:  %dmm\n", info->focal_length);
		fprintf(stream, "[/Image_Info]\n");
		return true;
	}
	
	return false;
}
