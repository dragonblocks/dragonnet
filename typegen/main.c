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

static void gen_serializers(FILE *c_fp, FILE *h_fp)
{
	fprintf(h_fp, "typedef char *string;\n");
	fprintf(h_fp, "typedef struct {\n\tu32 siz;\n\tu8 *data;\n} Blob;\n\n");

	for (u8 bits = 8; bits <= 64; bits *= 2) {
		char *fmt_u = "void send_u%d(DragonnetPeer *p, bool submit, u%d v)%s\n";
		char *fmt_s = "void send_s%d(DragonnetPeer *p, bool submit, s%d v)%s\n";

		fprintf(h_fp, fmt_u, bits, bits, ";");
		fprintf(h_fp, fmt_s, bits, bits, ";");

		fprintf(c_fp, fmt_u, bits, bits, "");
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\tu%d be = htobe%d(v);\n", bits, bits);
		fprintf(c_fp, "\tsend_raw(p, submit, &be, sizeof be);\n");
		fprintf(c_fp, "}\n\n");

		fprintf(c_fp, fmt_s, bits, bits, "");
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\tsend_u%d(p, submit, (u%d) v);\n", bits, bits);
		fprintf(c_fp, "}\n\n");

		if (bits >= 32) {
			char *fmt_f = "void send_f%d(DragonnetPeer *p, bool submit, f%d v)%s\n";
			fprintf(h_fp, fmt_f, bits, bits, ";");

			fprintf(c_fp, fmt_f, bits, bits, "");
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tsend_u%d(p, submit, (u%d) v);\n", bits, bits);
			fprintf(c_fp, "}\n\n");
		}
	}

	fprintf(h_fp, "\n");

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = "void send_v%du%d(DragonnetPeer *p, bool submit, v%du%d v)%s\n";
			char *fmt_s = "void send_v%ds%d(DragonnetPeer *p, bool submit, v%ds%d v)%s\n";

			fprintf(h_fp, fmt_u, elems, bits, elems, bits, ";");
			fprintf(h_fp, fmt_s, elems, bits, elems, bits, ";");

			fprintf(c_fp, fmt_u, elems, bits, elems, bits, "");
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
			fprintf(c_fp, "\t\tsend_u%d(p, (i == %d-1) ? submit : false, v[i]);\n", bits, elems);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			fprintf(c_fp, fmt_s, elems, bits, elems, bits, "");
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
			fprintf(c_fp, "\t\tsend_s%d(p, (i == %d-1) ? submit : false, v[i]);\n", bits, elems);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			if (bits >= 32) {
				char *fmt_f = "void send_v%df%d(DragonnetPeer *p, bool submit, v%df%d v)%s\n";
				fprintf(h_fp, fmt_f, elems, bits, elems, bits, ";");

				fprintf(c_fp, fmt_f, elems, bits, elems, bits, "");
				fprintf(c_fp, "{\n");
				fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
				fprintf(c_fp, "\t\tsend_s%d(p, (i == %d-1) ? submit : false, v[i]);\n", bits, elems);
				fprintf(c_fp, "\t}\n");
				fprintf(c_fp, "}\n\n");
			}
		}
	}

	fprintf(h_fp, "\n");

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = "void send_aabb%du%d(DragonnetPeer *p, bool submit, aabb%du%d v)\n";
			char *fmt_s = "void send_aabb%ds%d(DragonnetPeer *p, bool submit, aabb%ds%d v)\n";

			fprintf(h_fp, fmt_u, elems, bits, elems, bits, ";");
			fprintf(h_fp, fmt_s, elems, bits, elems, bits, ";");

			fprintf(c_fp, fmt_u, elems, bits, elems, bits, "");
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
			fprintf(c_fp, "\t\tsend_v%du%d(p, (i == 1) ? submit : false, v[i]);\n", elems, bits);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			fprintf(c_fp, fmt_s, elems, bits, elems, bits, "");
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
			fprintf(c_fp, "\t\tsend_v%ds%d(p, (i == 1) ? submit : false, v[i]);\n", elems, bits);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			if (bits >= 32) {
				char *fmt_f = "void send_aabb%df%d(DragonnetPeer *p, bool submit, aabb%df%d v);\n";
				fprintf(h_fp, fmt_f, elems, bits, elems, bits, ";");

				fprintf(c_fp, fmt_f, elems, bits, elems, bits, "");
				fprintf(c_fp, "{\n");
				fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
				fprintf(c_fp, "\t\tsend_v%df%d(p, (i == 1) ? submit : false, v[i]);\n", elems, bits);
				fprintf(c_fp, "\t}\n");
				fprintf(c_fp, "}\n\n");
			}
		}
	}

	fprintf(h_fp, "\n");

	char *fmt_str = "void send_string(DragonnetPeer *p, bool submit, string v)%s\n";
	char *fmt_blob = "void send_Blob(DragonnetPeer *p, bool submit, Blob *v)%s\n\n";

	fprintf(h_fp, fmt_str, ";");
	fprintf(h_fp, fmt_blob, ";");

	fprintf(c_fp, fmt_str, "");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tsend_raw(p, submit, v, strlen(v));\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, fmt_blob, "");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tsend_u32(p, false, v->siz);\n");
	fprintf(c_fp, "\tsend_raw(p, submit, v->data, v->siz);\n");
	fprintf(c_fp, "}\n\n");
}

static void gen_deserializers(FILE *c_fp, FILE *h_fp)
{
	c_fp = c_fp;
	for (u8 bits = 8; bits <= 64; bits *= 2) {
		char *fmt_u = "u%d recv_u%d(DragonnetPeer *p)%s\n";
		char *fmt_s = "s%d recv_s%d(DragonnetPeer *p)%s\n";

		fprintf(h_fp, fmt_u, bits, bits, ";");
		fprintf(h_fp, fmt_s, bits, bits, ";");

		fprintf(c_fp, fmt_u, bits, bits, "");
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\tu%d be = recv_raw(p, &be, sizeof be);\n", bits);
		fprintf(c_fp, "\treturn be%dtoh(be);\n", bits);
		fprintf(c_fp, "}\n\n");

		fprintf(c_fp, fmt_s, bits, bits, "");
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\treturn (s%d) recv_u%d(p);\n", bits, bits);
		fprintf(c_fp, "}\n\n");

		if (bits >= 32) {
			char *fmt_f = "f%d recv_f%d(DragonnetPeer *p)%s\n";
			fprintf(h_fp, fmt_f, bits, bits, ";");

			fprintf(c_fp, fmt_f, bits, bits, "");
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\treturn (f%d) recv_u%d(p);\n", bits, bits);
			fprintf(c_fp, "}\n\n");
		}
	}

	fprintf(h_fp, "\n");

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = "v%du%d recv_v%du%d(DragonnetPeer *p)%s\n";
			char *fmt_s = "v%ds%d recv_v%ds%d(DragonnetPeer *p)%s\n";

			fprintf(h_fp, fmt_u, elems, bits, elems, bits, ";");
			fprintf(h_fp, fmt_s, elems, bits, elems, bits, ";");

			fprintf(c_fp, fmt_u, elems, bits, elems, bits, "");
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tv%du%d v = {0};\n", elems, bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
			fprintf(c_fp, "\t\tv[i] = recv_u%d(p);\n", bits);
			fprintf(c_fp, "\t}\n\n");
			fprintf(c_fp, "\treturn v;\n");
			fprintf(c_fp, "}\n\n");

			fprintf(c_fp, fmt_s, elems, bits, elems, bits, "");
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tv%ds%d v = {0};\n", elems, bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
			fprintf(c_fp, "\t\tv[i] = recv_s%d(p);\n", bits);
			fprintf(c_fp, "\t}\n\n");
			fprintf(c_fp, "\treturn v;\n");
			fprintf(c_fp, "}\n\n");

			if (bits >= 32) {
				char *fmt_f = "v%df%d recv_v%df%d(DragonnetPeer *p)%s\n";
				fprintf(h_fp, fmt_f, elems, bits, elems, bits, ";");

				fprintf(c_fp, fmt_f, elems, bits, elems, bits, "");
				fprintf(c_fp, "{\n");
				fprintf(c_fp, "\tv%df%d v = {0};\n", elems, bits);
				fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
				fprintf(c_fp, "\t\tv[i] = recv_f%d(p);\n", bits);
				fprintf(c_fp, "\t}\n\n");
				fprintf(c_fp, "\treturn v;\n");
				fprintf(c_fp, "}\n\n");
			}
		}
	}

	fprintf(h_fp, "\n");

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = "aabb%du%d recv_aabb%du%d(DragonnetPeer *p)%s\n";
			char *fmt_s = "aabb%ds%d recv_aabb%ds%d(DragonnetPeer *p)%s\n";

			fprintf(h_fp, fmt_u, elems, bits, elems, bits, ";");
			fprintf(h_fp, fmt_s, elems, bits, elems, bits, ";");

			fprintf(c_fp, fmt_u, elems, bits, elems, bits, "");
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\taabb%du%d v = {0};\n", elems, bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
			fprintf(c_fp, "\t\tv[i] = recv_v%du%d(p);\n", elems, bits);
			fprintf(c_fp, "\t}\n\n");
			fprintf(c_fp, "\treturn v;\n");
			fprintf(c_fp, "}\n\n");

			fprintf(c_fp, fmt_s, elems, bits, elems, bits, "");
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\taabb%ds%d v = {0};\n", elems, bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
			fprintf(c_fp, "\t\tv[i] = recv_v%ds%d(p);\n", elems, bits);
			fprintf(c_fp, "\t}\n\n");
			fprintf(c_fp, "\treturn v;\n");
			fprintf(c_fp, "}\n\n");

			if (bits >= 32) {
				char *fmt_f = "aabb%df%d recv_aabb%df%d(DragonnetPeer *p)%s\n";
				fprintf(h_fp, fmt_f, elems, bits, elems, bits, ";");

				fprintf(c_fp, fmt_f, elems, bits, elems, bits, "");
				fprintf(c_fp, "{\n");
				fprintf(c_fp, "\taabb%df%d v = {0};\n", elems, bits);
				fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
				fprintf(c_fp, "\t\tv[i] = recv_v%ds%d(p);\n", elems, bits);
				fprintf(c_fp, "\t}\n\n");
				fprintf(c_fp, "\treturn v;\n");
				fprintf(c_fp, "}\n\n");
			}
		}
	}

	fprintf(h_fp, "\n");

	char *fmt_str = "string recv_string(DragonnetPeer *p)%s\n";
	char *fmt_blob = "Blob *recv_Blob(DragonnetPeer *p)%s\n\n";

	fprintf(h_fp, fmt_str, ";");
	fprintf(h_fp, fmt_blob, ";");

	fprintf(c_fp, fmt_str, "");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tstring v = malloc(sizeof(u16));\n\n");
	fprintf(c_fp, "\tchar ch;\n");
	fprintf(c_fp, "\tfor (u16 i = 0; ch != '\\0'; ++i) {\n");
	fprintf(c_fp, "\t\tch = recv_s8(p);\n");
	fprintf(c_fp, "\t\tv[i] = ch;\n");
	fprintf(c_fp, "\t}\n\n");
	fprintf(c_fp, "\tv = realloc(v, strlen(v));\n");
	fprintf(c_fp, "\treturn v;\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, fmt_blob, "");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tBlob *v = malloc(sizeof *v);\n");
	fprintf(c_fp, "\tv->siz = recv_u32(p, false, v->siz);\n");
	fprintf(c_fp, "\tv->data = malloc(v->siz);\n");
	fprintf(c_fp, "\trecv_raw(p, v->data, v->siz);\n\n");
	fprintf(c_fp, "\treturn v;\n");
	fprintf(c_fp, "}\n\n");
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
	fprintf(h_fp, "#define htobe8(x) (x)\n");
	fprintf(h_fp, "#define be8toh(x) (x)\n\n");

	gen_serializers(c_fp, h_fp);
	gen_deserializers(c_fp, h_fp);

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
