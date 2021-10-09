#include <ctype.h>
#include <dragonport/asprintf.h>
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

// Socket based

static void gen_serializers(FILE *c_fp)
{
	for (u8 bits = 8; bits <= 64; bits *= 2) {
		char *fmt_u = "__attribute__((unused)) static void send_u%d(DragonnetPeer *p, bool submit, u%d v)\n";
		char *fmt_s = "__attribute__((unused)) static void send_s%d(DragonnetPeer *p, bool submit, s%d v)\n";

		fprintf(c_fp, fmt_u, bits, bits);
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\tu%d be = htobe%d(v);\n", bits, bits);
		fprintf(c_fp, "\tdragonnet_send_raw(p, submit, &be, sizeof be);\n");
		fprintf(c_fp, "}\n\n");

		fprintf(c_fp, fmt_s, bits, bits, "");
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\tsend_u%d(p, submit, (u%d) v);\n", bits, bits);
		fprintf(c_fp, "}\n\n");

		if (bits >= 32) {
			char *fmt_f = "static void send_f%d(DragonnetPeer *p, bool submit, f%d v)\n";

			fprintf(c_fp, fmt_f, bits, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tsend_u%d(p, submit, (u%d) v);\n", bits, bits);
			fprintf(c_fp, "}\n\n");
		}
	}

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = "__attribute__((unused)) static void send_v%du%d(DragonnetPeer *p, bool submit, v%du%d v)\n";
			char *fmt_s = "__attribute__((unused)) static void send_v%ds%d(DragonnetPeer *p, bool submit, v%ds%d v)\n";

			fprintf(c_fp, fmt_u, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tu%d *ptr = &v.x;\n", bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
			fprintf(c_fp, "\t\tsend_u%d(p, (i == %d-1) ? submit : false, *ptr++);\n", bits, elems);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			fprintf(c_fp, fmt_s, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\ts%d *ptr = &v.x;\n", bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
			fprintf(c_fp, "\t\tsend_s%d(p, (i == %d-1) ? submit : false, *ptr++);\n", bits, elems);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			if (bits >= 32) {
				char *fmt_f = "__attribute__((unused)) static void send_v%df%d(DragonnetPeer *p, bool submit, v%df%d v)\n";

				fprintf(c_fp, fmt_f, elems, bits, elems, bits);
				fprintf(c_fp, "{\n");
				fprintf(c_fp, "\tf%d *ptr = &v.x;\n", bits);
				fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
				fprintf(c_fp, "\t\tsend_f%d(p, (i == %d-1) ? submit : false, *ptr++);\n", bits, elems);
				fprintf(c_fp, "\t}\n");
				fprintf(c_fp, "}\n\n");
			}
		}
	}

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = "__attribute__((unused)) static void send_aabb%du%d(DragonnetPeer *p, bool submit, aabb%du%d v)\n";
			char *fmt_s = "__attribute__((unused)) static void send_aabb%ds%d(DragonnetPeer *p, bool submit, aabb%ds%d v)\n";

			fprintf(c_fp, fmt_u, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tv%du%d *ptr = &v.min;\n", elems, bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
			fprintf(c_fp, "\t\tsend_v%du%d(p, (i == 1) ? submit : false, *ptr++);\n", elems, bits);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			fprintf(c_fp, fmt_s, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tv%ds%d *ptr = &v.min;\n", elems, bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
			fprintf(c_fp, "\t\tsend_v%ds%d(p, (i == 1) ? submit : false, *ptr++);\n", elems, bits);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			if (bits >= 32) {
				char *fmt_f = "__attribute__((unused)) static void send_aabb%df%d(DragonnetPeer *p, bool submit, aabb%df%d v)\n";

				fprintf(c_fp, fmt_f, elems, bits, elems, bits);
				fprintf(c_fp, "{\n");
				fprintf(c_fp, "\tv%df%d *ptr = &v.min;\n", elems, bits);
				fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
				fprintf(c_fp, "\t\tsend_v%df%d(p, (i == 1) ? submit : false, *ptr++);\n", elems, bits);
				fprintf(c_fp, "\t}\n");
				fprintf(c_fp, "}\n\n");
			}
		}
	}

	fprintf(c_fp, "__attribute__((unused)) static void send_string(DragonnetPeer *p, bool submit, string v)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tdragonnet_send_raw(p, submit, v, strlen(v));\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, "__attribute__((unused)) static void send_Blob(DragonnetPeer *p, bool submit, Blob *v)\n\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tsend_u32(p, false, v->siz);\n");
	fprintf(c_fp, "\tdragonnet_send_raw(p, submit, v->data, v->siz);\n");
	fprintf(c_fp, "}\n\n");
}

static void gen_deserializers(FILE *c_fp)
{
	for (u8 bits = 8; bits <= 64; bits *= 2) {
		char *fmt = "__attribute__((unused)) static void recv_n%d(DragonnetPeer *p, void *buf)\n";

		fprintf(c_fp, fmt, bits);
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\tu%d be;\n", bits);
		fprintf(c_fp, "\tdragonnet_recv_raw(p, &be, sizeof be);\n");
		fprintf(c_fp, "\tbe = be%dtoh(be);\n", bits);
		fprintf(c_fp, "\tmemcpy(buf, &be, sizeof be);\n");
		fprintf(c_fp, "}\n\n");
	}

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt = "__attribute__((unused)) static void recv_v%dn%d(DragonnetPeer *p, void *buf)\n";

			fprintf(c_fp, fmt, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i)\n", elems);
			fprintf(c_fp, "\t\trecv_n%d(p, buf);\n", bits);
			fprintf(c_fp, "}\n\n");
		}
	}

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt = "__attribute__((unused)) static void recv_aabb%dn%d(DragonnetPeer *p, void *buf)\n";

			fprintf(c_fp, fmt, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i)\n");
			fprintf(c_fp, "\t\trecv_v%dn%d(p, buf);\n", elems, bits);
			fprintf(c_fp, "}\n\n");
		}
	}

	fprintf(c_fp, "__attribute__((unused)) static void recv_string(DragonnetPeer *p, void *buf)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tstring v = malloc(1 + (u16) ~0);\n\n");
	fprintf(c_fp, "\tchar ch;\n");
	fprintf(c_fp, "\tfor (u16 i = 0; ch != '\\0'; ++i) {\n");
	fprintf(c_fp, "\t\trecv_n8(p, &ch);\n");
	fprintf(c_fp, "\t\tv[i] = ch;\n");
	fprintf(c_fp, "\t}\n\n");
	fprintf(c_fp, "\tv = realloc(v, strlen(v));\n");
	fprintf(c_fp, "\tmemcpy(buf, v, strlen(v));\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, "__attribute__((unused)) static void recv_Blob(DragonnetPeer *p, void *buf)\n\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tBlob *v = malloc(sizeof *v);\n");
	fprintf(c_fp, "\trecv_n32(p, &v->siz);\n");
	fprintf(c_fp, "\tv->data = malloc(v->siz);\n");
	fprintf(c_fp, "\tdragonnet_recv_raw(p, v->data, v->siz);\n\n");
	fprintf(c_fp, "\tmemcpy(buf, v->data, v->siz);\n");
	fprintf(c_fp, "}\n\n");
}

// Buffer based

static void gen_buffer_serializers(FILE *c_fp)
{
	for (u8 bits = 8; bits <= 64; bits *= 2) {
		char *fmt_u = "__attribute__((unused)) static void buf_write_u%d(u8 **buf, size_t *n, u%d v)\n";
		char *fmt_s = "__attribute__((unused)) static void buf_write_s%d(u8 **buf, size_t *n, s%d v)\n";

		fprintf(c_fp, fmt_u, bits, bits);
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\tu%d be = htobe%d(v);\n", bits, bits);
		fprintf(c_fp, "\tdragonnet_write_raw(buf, n, &be, sizeof be);\n");
		fprintf(c_fp, "}\n\n");

		fprintf(c_fp, fmt_s, bits, bits, "");
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\tbuf_write_u%d(buf, n, (u%d) v);\n", bits, bits);
		fprintf(c_fp, "}\n\n");

		if (bits >= 32) {
			char *fmt_f = "static void buf_write_f%d(u8 **buf, size_t *n, f%d v)\n";

			fprintf(c_fp, fmt_f, bits, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tbuf_write_u%d(buf, n, (u%d) v);\n", bits, bits);
			fprintf(c_fp, "}\n\n");
		}
	}

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = "__attribute__((unused)) static void buf_write_v%du%d(u8 **buf, size_t *n, v%du%d v)\n";
			char *fmt_s = "__attribute__((unused)) static void buf_write_v%ds%d(u8 **buf, size_t *n, v%ds%d v)\n";

			fprintf(c_fp, fmt_u, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tu%d *ptr = &v.x;\n", bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
			fprintf(c_fp, "\t\tbuf_write_u%d(buf, n, *ptr++);\n", bits);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			fprintf(c_fp, fmt_s, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\ts%d *ptr = &v.x;\n", bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
			fprintf(c_fp, "\t\tbuf_write_s%d(buf, n, *ptr++);\n", bits);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			if (bits >= 32) {
				char *fmt_f = "__attribute__((unused)) static void buf_write_v%df%d(u8 **buf, size_t *n, v%df%d v)\n";

				fprintf(c_fp, fmt_f, elems, bits, elems, bits);
				fprintf(c_fp, "{\n");
				fprintf(c_fp, "\tf%d *ptr = &v.x;\n", bits);
				fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
				fprintf(c_fp, "\t\tbuf_write_f%d(buf, n, *ptr++);\n", bits);
				fprintf(c_fp, "\t}\n");
				fprintf(c_fp, "}\n\n");
			}
		}
	}

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = "__attribute__((unused)) static void buf_write_aabb%du%d(u8 **buf, size_t *n, aabb%du%d v)\n";
			char *fmt_s = "__attribute__((unused)) static void buf_write_aabb%ds%d(u8 **buf, size_t *n, aabb%ds%d v)\n";

			fprintf(c_fp, fmt_u, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tv%du%d *ptr = &v.min;\n", elems, bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
			fprintf(c_fp, "\t\tbuf_write_v%du%d(buf, n, *ptr++);\n", elems, bits);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			fprintf(c_fp, fmt_s, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tv%ds%d *ptr = &v.min;\n", elems, bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
			fprintf(c_fp, "\t\tbuf_write_v%ds%d(buf, n, *ptr++);\n", elems, bits);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			if (bits >= 32) {
				char *fmt_f = "__attribute__((unused)) static void buf_write_aabb%df%d(u8 **buf, size_t *n, aabb%df%d v)\n";

				fprintf(c_fp, fmt_f, elems, bits, elems, bits);
				fprintf(c_fp, "{\n");
				fprintf(c_fp, "\tv%df%d *ptr = &v.min;\n", elems, bits);
				fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
				fprintf(c_fp, "\t\tbuf_write_v%df%d(buf, n, *ptr++);\n", elems, bits);
				fprintf(c_fp, "\t}\n");
				fprintf(c_fp, "}\n\n");
			}
		}
	}

	fprintf(c_fp, "__attribute__((unused)) static void buf_write_string(u8 **buf, size_t *n, string v)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tdragonnet_write_raw(buf, n, v, strlen(v));\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, "__attribute__((unused)) static void buf_write_Blob(u8 **buf, size_t *n, Blob *v)\n\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tbuf_write_u32(buf, n, v->siz);\n");
	fprintf(c_fp, "\tdragonnet_write_raw(buf, n, v->data, v->siz);\n");
	fprintf(c_fp, "}\n\n");
}

static void gen_buffer_deserializers(FILE *c_fp)
{
	for (u8 bits = 8; bits <= 64; bits *= 2) {
		char *fmt_u = "__attribute__((unused)) static u%d buf_read_u%d(u8 **buf, size_t *n)\n";
		char *fmt_s = "__attribute__((unused)) static s%d buf_read_s%d(u8 **buf, size_t *n)\n";

		fprintf(c_fp, fmt_u, bits, bits);
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\tu%d be;\n", bits);
		fprintf(c_fp, "\tdragonnet_read_raw(buf, n, &be, sizeof be);\n");
		fprintf(c_fp, "\treturn be%dtoh(be);\n", bits);
		fprintf(c_fp, "}\n\n");

		fprintf(c_fp, fmt_s, bits, bits);
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\treturn (s%d) buf_read_u%d(buf, n);\n", bits, bits);
		fprintf(c_fp, "}\n\n");

		if (bits >= 32) {
			char *fmt_f = "__attribute__((unused)) static f%d buf_read_f%d(u8 **buf, size_t *n)\n";

			fprintf(c_fp, fmt_f, bits, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\treturn (f%d) buf_read_u%d(buf, n);\n", bits, bits);
			fprintf(c_fp, "}\n\n");
		}
	}

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = "__attribute__((unused)) static v%du%d buf_read_v%du%d(u8 **buf, size_t *n)\n";
			char *fmt_s = "__attribute__((unused)) static v%ds%d buf_read_v%ds%d(u8 **buf, size_t *n)\n";

			fprintf(c_fp, fmt_u, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tv%du%d v = {0};\n", elems, bits);
			fprintf(c_fp, "\tu%d *ptr = &v.x;\n\n", bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
			fprintf(c_fp, "\t\t*ptr++ = buf_read_u%d(buf, n);\n", bits);
			fprintf(c_fp, "\t}\n\n");
			fprintf(c_fp, "\treturn v;\n");
			fprintf(c_fp, "}\n\n");

			fprintf(c_fp, fmt_s, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tv%ds%d v = {0};\n", elems, bits);
			fprintf(c_fp, "\ts%d *ptr = &v.x;\n\n", bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
			fprintf(c_fp, "\t\t*ptr++ = buf_read_s%d(buf, n);\n", bits);
			fprintf(c_fp, "\t}\n\n");
			fprintf(c_fp, "\treturn v;\n");
			fprintf(c_fp, "}\n\n");

			if (bits >= 32) {
				char *fmt_f = "__attribute__((unused)) static v%df%d buf_read_v%df%d(u8 **buf, size_t *n)\n";

				fprintf(c_fp, fmt_f, elems, bits, elems, bits);
				fprintf(c_fp, "{\n");
				fprintf(c_fp, "\tv%df%d v = {0};\n", elems, bits);
				fprintf(c_fp, "\tf%d *ptr = &v.x;\n\n", bits);
				fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
				fprintf(c_fp, "\t\t*ptr++ = buf_read_f%d(buf, n);\n", bits);
				fprintf(c_fp, "\t}\n\n");
				fprintf(c_fp, "\treturn v;\n");
				fprintf(c_fp, "}\n\n");
			}
		}
	}

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = "__attribute__((unused)) static aabb%du%d buf_read_aabb%du%d(u8 **buf, size_t *n)\n";
			char *fmt_s = "__attribute__((unused)) static aabb%ds%d buf_read_aabb%ds%d(u8 **buf, size_t *n)\n";

			fprintf(c_fp, fmt_u, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\taabb%du%d v = {0};\n", elems, bits);
			fprintf(c_fp, "\tv%du%d *ptr = &v.min;\n\n", elems, bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
			fprintf(c_fp, "\t\t*ptr++ = buf_read_v%du%d(buf, n);\n", elems, bits);
			fprintf(c_fp, "\t}\n\n");
			fprintf(c_fp, "\treturn v;\n");
			fprintf(c_fp, "}\n\n");

			fprintf(c_fp, fmt_s, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\taabb%ds%d v = {0};\n", elems, bits);
			fprintf(c_fp, "\tv%ds%d *ptr = &v.min;\n\n", elems, bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
			fprintf(c_fp, "\t\t*ptr++ = buf_read_v%ds%d(buf, n);\n", elems, bits);
			fprintf(c_fp, "\t}\n\n");
			fprintf(c_fp, "\treturn v;\n");
			fprintf(c_fp, "}\n\n");

			if (bits >= 32) {
				char *fmt_f = "__attribute__((unused)) static aabb%df%d buf_read_aabb%df%d(u8 **buf, size_t *n)\n";

				fprintf(c_fp, fmt_f, elems, bits, elems, bits);
				fprintf(c_fp, "{\n");
				fprintf(c_fp, "\taabb%df%d v = {0};\n", elems, bits);
				fprintf(c_fp, "\tv%df%d *ptr = &v.min;\n\n", elems, bits);
				fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
				fprintf(c_fp, "\t\t*ptr++ = buf_read_v%df%d(buf, n);\n", elems, bits);
				fprintf(c_fp, "\t}\n\n");
				fprintf(c_fp, "\treturn v;\n");
				fprintf(c_fp, "}\n\n");
			}
		}
	}

	fprintf(c_fp, "__attribute__((unused)) static string buf_read_string(u8 **buf, size_t *n)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tstring v = malloc(sizeof(u16));\n\n");
	fprintf(c_fp, "\tchar ch;\n");
	fprintf(c_fp, "\tfor (u16 i = 0; ch != '\\0'; ++i) {\n");
	fprintf(c_fp, "\t\tch = buf_read_s8(buf, n);\n");
	fprintf(c_fp, "\t\tv[i] = ch;\n");
	fprintf(c_fp, "\t}\n\n");
	fprintf(c_fp, "\tv = realloc(v, strlen(v));\n");
	fprintf(c_fp, "\treturn v;\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, "__attribute__((unused)) static Blob *buf_read_Blob(u8 **buf, size_t *n)\n\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tBlob *v = malloc(sizeof *v);\n");
	fprintf(c_fp, "\tv->siz = buf_read_u32(buf, n);\n");
	fprintf(c_fp, "\tv->data = malloc(v->siz);\n");
	fprintf(c_fp, "\tdragonnet_read_raw(buf, n, v->data, v->siz);\n\n");
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
	fprintf(c_fp, "#include <stdlib.h>\n");
	fprintf(c_fp, "#include <string.h>\n");
	fprintf(c_fp, "#include <dragonnet/recv.h>\n");
	fprintf(c_fp, "#include <dragonnet/send.h>\n\n");
	fprintf(c_fp, "#include \"dnet-types.h\"\n\n");

	FILE *h_fp = fopen("dnet-types.h", "w");
	fprintf(h_fp, "#include <dragontype/number.h>\n\n");
	fprintf(h_fp, "#define htobe8(x) (x)\n");
	fprintf(h_fp, "#define be8toh(x) (x)\n\n");
	fprintf(h_fp, "typedef char *string;\n");
	fprintf(h_fp, "typedef struct {\n\tu32 siz;\n\tu8 *data;\n} Blob;\n\n");

	gen_serializers(c_fp);
	gen_deserializers(c_fp);

	gen_buffer_serializers(c_fp);
	gen_buffer_deserializers(c_fp);

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
			fprintf(c_fp, "__attribute__((unused)) static void send_%s(DragonnetPeer *p, %s type)\n{\n", msg, msg);
		} else {
			char **tokens;
			size_t tokens_len = split(&tokens, msgs[i], " ");

			fprintf(c_fp, "\tsend_%s(p, false, type.%s);\n", &tokens[0][1], tokens[1]);
			free_split(tokens, tokens_len);
			tokens = NULL;
		}
	}

	fprintf(c_fp, "}\n\n");
	msg = NULL;

	for (size_t i = 0; i < msgs_len; ++i) {
		if (msgs[i][0] != '\t') {
			if (msg != NULL)
				fprintf(c_fp, "}\n\n");

			msg = msgs[i];
			fprintf(h_fp, "void dragonnet_peer_send_%s(DragonnetPeer *p, %s *type);\n", msg, msg);
			fprintf(c_fp, "void dragonnet_peer_send_%s(DragonnetPeer *p, %s *type)\n{\n", msg, msg);

			char upper[1 + strlen(msgs[i])];
			char *ptr = upper;
			strcpy(upper, msg);

			while ((*ptr = *ptr ? toupper(*ptr) : '\0'))
				++ptr;

			fprintf(c_fp, "\tsend_u16(p, false, DRAGONNET_TYPE_%s);\n", upper);
		} else {
			char **tokens;
			size_t tokens_len = split(&tokens, msgs[i], " ");

			if (i >= msgs_len-1 || msgs[1+i][0] != '\t')
				fprintf(c_fp, "\tsend_%s(p, true, type->%s);\n", &tokens[0][1], tokens[1]);
			else
				fprintf(c_fp, "\tsend_%s(p, false, type->%s);\n", &tokens[0][1], tokens[1]);

			free_split(tokens, tokens_len);
			tokens = NULL;
		}
	}

	fprintf(c_fp, "}\n\n");
	msg = NULL;

	for (size_t i = 0; i < msgs_len; ++i) {
		if (msgs[i][0] != '\t') {
			if (msg != NULL) {
				fprintf(c_fp, "}\n\n");
			}

			msg = msgs[i];
			fprintf(c_fp, "static void dragonnet_peer_recv_%s(DragonnetPeer *p, void *buf)\n{\n", msg);
			fprintf(c_fp, "\t%s *type = (%s *) buf;\n", msg, msg);
		} else {
			char **tokens;
			size_t tokens_len = split(&tokens, msgs[i], " ");

			char type[strlen(&tokens[0][1])];
			strcpy(type, &tokens[0][1]);

			for (size_t bits = 8; bits <= 64; bits *= 2) {
				const char *fmt[] = {"u%d", "s%d", "f%d"};
				for (size_t j = 0; j < sizeof fmt / sizeof *fmt; ++j) {
					char *cmp;
					asprintf(&cmp, fmt[j], bits);

					if (strcmp(type, cmp) == 0)
						sprintf(type, "n%ld", bits);

					free(cmp);
				}
			}

			fprintf(c_fp, "\trecv_%s(p, &type->%s);\n", type, tokens[1]);
			free_split(tokens, tokens_len);
			tokens = NULL;
		}
	}

	fprintf(c_fp, "}\n");
	msg = NULL;

	// Buffer (de)serialization
	for (size_t i = 0; i < msgs_len; ++i) {
		if (msgs[i][0] != '\t') {
			if (msg != NULL)
				fprintf(c_fp, "}\n\n");

			msg = msgs[i];
			fprintf(c_fp, "void dragonnet_buf_write_%s(u8 **buf, size_t *n, %s type)\n{\n", msg, msg);
		} else {
			char **tokens;
			size_t tokens_len = split(&tokens, msgs[i], " ");

			fprintf(c_fp, "\tbuf_write_%s(buf, n, type.%s);\n", &tokens[0][1], tokens[1]);

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
			fprintf(h_fp, "%s dragonnet_buf_read_%s(u8 **buf, size_t *n);\n", msg, msg);
			fprintf(c_fp, "%s dragonnet_buf_read_%s(u8 **buf, size_t *n)\n{\n", msg, msg);
			fprintf(c_fp, "\t%s type = {0};\n", msg);
		} else {
			char **tokens;
			size_t tokens_len = split(&tokens, msgs[i], " ");

			fprintf(c_fp, "\ttype.%s = buf_read_%s(buf, n);\n", tokens[1], &tokens[0][1]);
			free_split(tokens, tokens_len);
			tokens = NULL;
		}
	}

	fprintf(h_fp, "\n");
	fprintf(c_fp, "\treturn type;\n");
	fprintf(c_fp, "}\n\n");
	msg = NULL;

	// Create type enum
	fprintf(h_fp, "typedef enum {\n");
	for (size_t i = 0; i < msgs_len; ++i) {
		if (msgs[i][0] == '\t')
			continue;

		char upper[1 + strlen(msgs[i])];
		char *ptr = upper;
		strcpy(upper, msgs[i]);

		while ((*ptr = *ptr ? toupper(*ptr) : '\0'))
			++ptr;

		fprintf(h_fp, "\tDRAGONNET_TYPE_%s,\n", upper);
	}

	fprintf(h_fp, "\tDRAGONNET_NUM_TYPES\n");
	fprintf(h_fp, "} DragonnetTypeNum;\n");

	// ABI
	fprintf(c_fp, "u16 dragonnet_num_types = DRAGONNET_NUM_TYPES;\n");
	fprintf(c_fp, "DragonnetType dragonnet_types[] = {\n");

	for (size_t i = 0; i < msgs_len; ++i) {
		if (msgs[i][0] == '\t')
			continue;

		fprintf(c_fp, "\t{\n");
		fprintf(c_fp, "\t\t.siz = sizeof(%s),\n", msgs[i]);
		fprintf(c_fp, "\t\t.deserialize = &dragonnet_peer_recv_%s\n", msgs[i]);
		fprintf(c_fp, "\t},\n");
	}

	fprintf(c_fp, "};\n");

	free_split(msgs, msgs_len);
	msgs = NULL;

	fclose(c_fp);
	fclose(h_fp);
	c_fp = NULL;
	h_fp = NULL;
}
