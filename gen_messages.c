#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t split(char ***strs, char *s, const char *delim)
{
	size_t i = 0;
	*strs = malloc((1+i) * sizeof(char *));

	char *tok = strtok(s, delim);
	while (tok != NULL) {
		*strs = realloc(*strs, (1+i) * sizeof(char *));
		(*strs)[i++] = tok;
		tok = strtok(NULL, delim);
	}

	return i;
}

int main(int argc, char **argv)
{
	FILE *fp = fopen("messages.dnet", "r");

	char data[1 << 16];
	memset(data, 0, sizeof data);
	fread(data, sizeof *data, sizeof data, fp);

	fclose(fp);
	fp = fopen("messages.h", "w");
	fprintf(fp, "#include <stdint.h>\n\n");

	char **msgs;
	size_t msgs_len = split(&msgs, data, "\n");

	char *msg = NULL;
	for (size_t i = 0; i < msgs_len; i++) {
		if (msgs[i][0] != '\t') {
			if (msg != NULL)
				fprintf(fp, "} %s;\n", msg);

			msg = msgs[i];
			fprintf(fp, "typedef struct {\n");
		} else {
			char **tokens;
			split(&tokens, msgs[i], " ");

			char type[10];
			if (strcmp(tokens[0], "\ts8") == 0)
				strcpy(type, "int8_t ");
			else if (strcmp(tokens[0], "\ts16") == 0)
				strcpy(type, "int16_t ");
			else if (strcmp(tokens[0], "\ts32") == 0)
				strcpy(type, "int32_t ");
			else if (strcmp(tokens[0], "\ts64") == 0)
				strcpy(type, "int64_t ");
			else if (strcmp(tokens[0], "\tu8") == 0)
				strcpy(type, "uint8_t ");
			else if (strcmp(tokens[0], "\tu16") == 0)
				strcpy(type, "uint16_t ");
			else if (strcmp(tokens[0], "\tu32") == 0)
				strcpy(type, "uint32_t ");
			else if (strcmp(tokens[0], "\tu64") == 0)
				strcpy(type, "uint64_t ");

			fprintf(fp, "\t%s%s;\n", type, tokens[1]);
			free(tokens);
		}
	}

	fprintf(fp, "} %s;\n", msg);

	free(msgs);
	fclose(fp);
}
