#include <ctype.h>
#include <dragontype/number.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t split(char ***strs, char *s, const char *delim)
{
	size_t i = 0;
	*strs = malloc((1+i) * sizeof(char *));

	// Can't be allocated on the stack for some reason
	char *str = malloc(1+strlen(s));
	strcpy(str, s);

	char *tok = strtok(str, delim);
	while (tok != NULL) {
		*strs = realloc(*strs, (1+i) * sizeof(char *));
		(*strs)[i++] = strdup(tok);
		tok = strtok(NULL, delim);
	}

	free(str);
	return i;
}

static void free_split(char **strs, size_t n)
{
	for (size_t i = 0; i < n; ++i)
		free(strs[i]);

	free(strs);
}

int main(__attribute((unused)) int argc, __attribute((unused)) char **argv)
{
	FILE *fp = fopen("types.dnet", "r");

	char data[1 << 16];
	memset(data, '\0', sizeof data);
	fread(data, sizeof *data, sizeof data, fp);

	fclose(fp);
	fp = NULL;

	FILE *c_fp = fopen("dnet-types.c", "w");
	fprintf(c_fp, "#include <dragonnet/recv.h>\n");
	fprintf(c_fp, "#include <dragonnet/send.h>\n\n");
	fprintf(c_fp, "#include \"dnet-types.h\"\n\n");

	FILE *h_fp = fopen("dnet-types.h", "w");
	fprintf(h_fp, "#include <dragontype/number.h>\n\n");

	char **msgs;
	size_t msgs_len = split(&msgs, data, "\n");

	// Create data types
	char *msg = NULL;
	for (size_t i = 0; i < msgs_len; ++i) {
		if (msgs[i][0] != '\t') {
			if (msg != NULL)
				fprintf(h_fp, "} %s;\n\n", msg);

			msg = msgs[i];
			fprintf(h_fp, "typedef struct {\n");
		} else {
			char **tokens;
			size_t tokens_len = split(&tokens, msgs[i], " ");

			fprintf(h_fp, "\t%s %s;\n", &tokens[0][1], tokens[1]);
			free_split(tokens, tokens_len);
			tokens = NULL;
		}
	}

	fprintf(h_fp, "} %s;\n\n", msg);
	msg = NULL;

	// Create (de)serialization functions
	for (size_t i = 0; i < msgs_len; ++i) {
		if (msgs[i][0] != '\t') {
			if (msg != NULL)
				fprintf(c_fp, "}\n\n");

			msg = msgs[i];
			fprintf(c_fp, "void dragonnet_send_%s(DragonnetPeer *p, %s type)\n{\n", msg, msg);
		} else {
			char **tokens;
			size_t tokens_len = split(&tokens, msgs[i], " ");

			if (i >= msgs_len-1 || msgs[1+i][0] != '\t')
				fprintf(c_fp, "\tsend_%s(p, true, type.%s);\n", &tokens[0][1], tokens[1]);
			else
				fprintf(c_fp, "\tsend_%s(p, false, type.%s);\n", &tokens[0][1], tokens[1]);

			free_split(tokens, tokens_len);
			tokens = NULL;
		}
	}

	fprintf(c_fp, "}\n\n");
	msg = NULL;

	for (size_t i = 0; i < msgs_len; ++i) {
		if (msgs[i][0] != '\t') {
			if (msg != NULL) {
				fprintf(c_fp, "\treturn type;\n");
				fprintf(c_fp, "}\n\n");
			}

			msg = msgs[i];
			fprintf(c_fp, "%s dragonnet_recv_%s(DragonnetPeer *p)\n{\n", msg, msg);
			fprintf(c_fp, "\t%s type = {0};\n", msg);
		} else {
			char **tokens;
			size_t tokens_len = split(&tokens, msgs[i], " ");

			fprintf(c_fp, "\ttype.%s = recv_%s(p);\n", tokens[1], &tokens[0][1]);
			free_split(tokens, tokens_len);
			tokens = NULL;
		}
	}

	fprintf(c_fp, "\treturn type;\n");
	fprintf(c_fp, "}\n");
	msg = NULL;

	// Create type enum
	size_t last_msg = 0;
	for (size_t i = 0; i < msgs_len; ++i)
		if (msgs[i][0] != '\t')
			last_msg = i;

	fprintf(h_fp, "typedef enum {\n");
	for (size_t i = 0; i < msgs_len; ++i) {
		if (msgs[i][0] == '\t')
			continue;

		char upper[1 + strlen(msgs[i])];
		char *ptr = upper;
		strcpy(upper, msgs[i]);

		while ((*ptr = *ptr ? toupper(*ptr) : '\0'))
			++ptr;

		if (i == last_msg)
			fprintf(h_fp, "\tDRAGONNET_TYPE_%s\n", upper);
		else
			fprintf(h_fp, "\tDRAGONNET_TYPE_%s,\n", upper);
	}

	fprintf(h_fp, "} DragonnetType;\n");

	free_split(msgs, msgs_len);
	msgs = NULL;

	fclose(c_fp);
	fclose(h_fp);
	c_fp = NULL;
	h_fp = NULL;
}
