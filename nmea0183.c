/*
 * getaround coding exercise
 * Bruce Bernstein
 * Oct 4, 2015
 */

#include <stdio.h>
#include <string.h>

typedef enum { false, true } bool;


#define MAX_SENTENCE_SIZE 256
#define MIN_SENTENCE_SIZE 6
#define TALKER_ID_SIZE 2
#define TALKER_ID_START 1
#define COMMON_COMMAND_ID_SIZE 3
#define COMMON_COMMAND_ID_START 3
#define TIME_INDEX 1
#define LATITUDE_INDEX 3
#define LATITUDE_HEMISPHERE_INDEX 4
#define LONGITUDE_INDEX 5
#define LONGITUDE_HEMISPHERE_INDEX 6

/* Implement my own line getter in case there's a libc issue with CRLF parsing
    Reads from stdin as a stream.  If we need a different pipe, this is the routine to fix. */
int GetLineOfCharsFromStdinTerminatedByCRLF(char *line, int n) {

	int len = 0;
	int inputChar;
	bool finished = false;

	while (finished == false) {
		inputChar = fgetc(stdin);
		if (inputChar == EOF) {
			finished = true;
			if (ferror(stdin)) {
				fprintf(stderr, "Error reading from stdin\n");
			}
		} else {
			if (inputChar == '\r') {
				/* Got CR, check for and flush LF */
				inputChar = fgetc(stdin);
				if (inputChar != '\n') {
					line[len] = '\0';
					fprintf(stderr, "Malformed input string %s of size %d, ignoring\n", line, len);
					len = 0;
				} else {
					line[len] = '\0';
					finished = true;
				}
			} else if (inputChar == '\n') {
				line[len] = '\0';
				fprintf(stderr, "input string %s of size %d missing <CR>\n", line, len);
				finished = true;
			} else {
				if (len >= n) {
					fprintf(stderr, "Input line of size %d is too long, ignoring\n", len);
					while ((inputChar != EOF) && (char)inputChar != '\n') {
						inputChar = fgetc(stdin);
					}
					len = 0;
				} else {
					/* An input line is too long, flush it and ignore it */
					line[len] = (char)inputChar;
					len += 1;
				}
			}
		}
	}
	return len;
}

/* Check the checksum.  While checksums are optional in NMEA 0183, they are required for the RMC sentences */
bool ValidateChecksumForInputSentence(char *sentence, int len) {
	int calculatedChecksum = 0;
	int suppliedChecksum = 0;
	bool result = false;

	if (sentence[0] == '$') {
		int i = 1;
		for (; (i < len) && (sentence[i] != '*'); i++) {
			calculatedChecksum ^= (int)(sentence[i]);
		}
		if (sentence[i] == '*') {
			sscanf(&sentence[i+1], "%x", &suppliedChecksum);
			if (suppliedChecksum != calculatedChecksum) {
				fprintf(stderr, "checksum mismatch, supplied %d, calculated %d, sentence:\n%s\n", suppliedChecksum, calculatedChecksum, sentence);
			} else {
				result = true;
			}
		}
	}
	return result;
}

/* General talker checker.   Use this to make mission creep easier */
bool CheckForTalker(const char *sentence, int len, const char *device) {
	bool result = false;
	if (len > MIN_SENTENCE_SIZE) {
		if (strlen(device) == TALKER_ID_SIZE) {
			if (strncmp(&sentence[TALKER_ID_START], device, TALKER_ID_SIZE) == 0) {
				result = true;
			}
		}
	}
	return result;
}

/* General sentence identifier checker.   Use this to make mission creep easier */
bool CheckSentenceIdentifier(const char *sentence, int len, const char *identifier) {
	bool result = false;
	if (len > MIN_SENTENCE_SIZE) {
		if (strlen(identifier) == COMMON_COMMAND_ID_SIZE) {
			if (strncmp(&sentence[COMMON_COMMAND_ID_START], identifier, COMMON_COMMAND_ID_SIZE) == 0) {
				result = true;
			}
		}
	}
	return result;
}

/*
 * I chose not to stream process the parameters, but rather index process them.
 * This enhances flexability with a performance cost of rescanning for each parameter.
 * If this becomes an issue, the approach can be re-thought.
 */
bool ReturnIndexedParameterFromSentence(int index, const char *sentence, int sentenceSize, char *returnBuffer, int bufferSize) {
	bool result = false;
	int scannedIndex = 0;
	int i = 0;
	while ((i < sentenceSize) && (scannedIndex < index)) {
		while ((i < sentenceSize) && (sentence[i++] != ',' )) {	}
		scannedIndex += 1;
	}
	if (i < sentenceSize) {
		int j = 0;
		while ((i < sentenceSize) && (j < bufferSize) && (sentence[i] != ',')) {
			returnBuffer[j++] = sentence[i++];
		}
		if (j < bufferSize) {
			returnBuffer[j] = '\0';
			result = true;
		}
	}
	return result;
}

void main(int argc, char *argv) {
	char inputSentence[MAX_SENTENCE_SIZE];
	int sentenceSize = 0;
	/* For expediency, use fixed size buffers.  If necessary for flexibility, implement a target measurement and allocate the buffers. */
	char timeBuffer[15];
	char latitudeBuffer[30];
	float latitudeMinutes;
	int latitudeDecimalFraction;
	char latitudeHemisphere[2];
	int latitudeDegrees;
	char longitudeBuffer[30];
	int longitudeDegrees;
	char longitudeHemisphere[2];
	float longitudeMinutes;
	int longitudeDecimalFraction;
	char resultLine[40];
	int outputCounter=0;
	bool finished=false;

	while ((!finished) && ((sentenceSize = GetLineOfCharsFromStdinTerminatedByCRLF(inputSentence, sizeof(inputSentence))) > 0)) {
		if (CheckForTalker(inputSentence, sentenceSize, "GP")) {
			if (CheckSentenceIdentifier(inputSentence, sentenceSize, "RMC")) {
				if (ValidateChecksumForInputSentence(inputSentence, sizeof(inputSentence))) {
					/* fprintf(stdout, "Input - %s\n", inputSentence); */
					if (ReturnIndexedParameterFromSentence(TIME_INDEX, inputSentence, sizeof(inputSentence), timeBuffer, sizeof(timeBuffer))) {
						if (ReturnIndexedParameterFromSentence(LATITUDE_INDEX, inputSentence, sizeof(inputSentence), latitudeBuffer, sizeof(latitudeBuffer))) {
							if (ReturnIndexedParameterFromSentence(LONGITUDE_INDEX, inputSentence, sizeof(inputSentence), longitudeBuffer, sizeof(longitudeBuffer))) {
								if (ReturnIndexedParameterFromSentence(LATITUDE_HEMISPHERE_INDEX, inputSentence, sizeof(inputSentence), latitudeHemisphere, sizeof(latitudeHemisphere))) {
									if (ReturnIndexedParameterFromSentence(LONGITUDE_HEMISPHERE_INDEX, inputSentence, sizeof(inputSentence), longitudeHemisphere, sizeof(longitudeHemisphere))) {
										/*
										 * All of the examples of NMEA 0183 streams I have seen have zero padding and predictable field sizes, so 
										 *  I have hard coded the positions within the fields.
										 */
										sscanf(&latitudeBuffer[2], "%f", &latitudeMinutes);
										latitudeBuffer[2] = '\0';
										sscanf(latitudeBuffer, "%d", &latitudeDegrees);
										latitudeDecimalFraction = (int)((latitudeMinutes*1000000)/60);
										sscanf(&longitudeBuffer[3], "%f", &longitudeMinutes);
										longitudeBuffer[3] = '\0';
										sscanf(longitudeBuffer, "%d", &longitudeDegrees);
										longitudeDecimalFraction = (int)((longitudeMinutes*1000000)/60);
										sprintf(resultLine, "%c%c:%c%c:%c%c, %s%d.%d, %s%d.%d",
											timeBuffer[0], timeBuffer[1],
											timeBuffer[2], timeBuffer[3],
											timeBuffer[4], timeBuffer[5],
											(latitudeHemisphere[0] == 'N')?"":"-", latitudeDegrees, latitudeDecimalFraction,
											(longitudeHemisphere[0] == 'E')?"":"-", longitudeDegrees, longitudeDecimalFraction);
										fprintf(stdout, "%s\n", resultLine);
										if (++outputCounter >= 100) {
											finished = true;
										}
									}
								}						
							}
						}
					}
				}
			}
		}
	}
}
