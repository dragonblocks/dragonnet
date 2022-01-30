#include <ctype.h>
#include <dragonport/asprintf.h>
#include <dragontype/number.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FUNC "__attribute__((unused)) static inline "

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

#define INDENT for (size_t i = 0; i < lvls + 1; i++) fprintf(fp, "\t")

static char *process_array(FILE *fp, char *src)
{
	size_t lvls = 0;

	size_t len = 1;
	char *str = malloc(1);

	*str = '\0';

	for (; *src != '\0'; src++) {
		if (*src == '[') {
			*src++ = '\0';

			size_t arrlen;
			src += sscanf(src, "%lu", &arrlen);

			INDENT; fprintf(fp, "for (size_t i%lu = 0; i%lu < %lu; i%lu++)\n", lvls, lvls, arrlen, lvls);

			char *buf;
			str = realloc(str, len += asprintf(&buf, "[i%lu]", lvls));
			strcat(str, buf);
			free(buf);

			lvls++;
		}
	}

	INDENT; return str;
}

#undef INDENT

// Socket based

static void gen_serializers(FILE *c_fp)
{
	fprintf(c_fp, FUNC "void raw_buf_read(Blob blob, void *data, size_t len)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tmemcpy(data, blob->data, len);\n");
	fprintf(c_fp, "\tmemcpy(blob->data, &blob->data[len], -len + blob->siz);\n");
	fprintf(c_fp, "\tblob->data = realloc(blob->data, -len + blob->siz);\n");
	fprintf(c_fp, "\tblob->siz -= len;\n");
	fprintf(c_fp, "\t}\n\n");

	fprintf(c_fp, FUNC "void raw_buf_write(Blob blob, const void *data, size_t len)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tblob->data = realloc(blob->data, len + blob->siz);\n");
	fprintf(c_fp, "\tmemcpy(&blob->data[blob->siz], data, len);\n");
	fprintf(c_fp, "\tblob->siz += len;\n");
	fprintf(c_fp, "}\n\n");

	for (u8 bits = 8; bits <= 64; bits *= 2) {
		char *fmt_u = FUNC "void send_u%d(DragonnetPeer *p, bool submit, u%d v)\n";
		char *fmt_s = FUNC "void send_s%d(DragonnetPeer *p, bool submit, s%d v)\n";

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
			char *fmt_f = FUNC "void send_f%d(DragonnetPeer *p, bool submit, f%d v)\n";

			fprintf(c_fp, fmt_f, bits, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tsend_u%d(p, submit, (u%d) v);\n", bits, bits);
			fprintf(c_fp, "}\n\n");
		}
	}

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = FUNC "void send_v%du%d(DragonnetPeer *p, bool submit, v%du%d v)\n";
			char *fmt_s = FUNC "void send_v%ds%d(DragonnetPeer *p, bool submit, v%ds%d v)\n";

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
				char *fmt_f = FUNC "void send_v%df%d(DragonnetPeer *p, bool submit, v%df%d v)\n";

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
			char *fmt_u = FUNC "void send_aabb%du%d(DragonnetPeer *p, bool submit, aabb%du%d v)\n";
			char *fmt_s = FUNC "void send_aabb%ds%d(DragonnetPeer *p, bool submit, aabb%ds%d v)\n";

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
				char *fmt_f = FUNC "void send_aabb%df%d(DragonnetPeer *p, bool submit, aabb%df%d v)\n";

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

	fprintf(c_fp, FUNC "void send_string(DragonnetPeer *p, bool submit, string v)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tdragonnet_send_raw(p, submit, v, strlen(v));\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, FUNC "void send_Blob(DragonnetPeer *p, bool submit, Blob v)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tsend_u32(p, false, v->siz);\n");
	fprintf(c_fp, "\tdragonnet_send_raw(p, submit, v->data, v->siz);\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, FUNC "void send_CompressedBlob(DragonnetPeer *p, bool submit, CompressedBlob v)\n\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tchar compr[2 + v->blob->siz];\n\n");
	fprintf(c_fp, "\tz_stream s;\n");
	fprintf(c_fp, "\ts.zalloc = Z_NULL;\n");
	fprintf(c_fp , "\ts.zfree = Z_NULL;\n");
	fprintf(c_fp, "\ts.opaque = Z_NULL;\n\n");
	fprintf(c_fp, "\ts.avail_in = v->blob->siz;\n");
	fprintf(c_fp, "\ts.avail_out = 3 + v->blob->siz;\n");
	fprintf(c_fp, "\ts.next_in = (Bytef *) v->blob->data;\n");
	fprintf(c_fp, "\ts.next_out = (Bytef *) compr;\n\n");
	fprintf(c_fp, "\tdeflateInit(&s, Z_BEST_COMPRESSION);\n");
	fprintf(c_fp, "\tdeflate(&s, Z_FINISH);\n");
	fprintf(c_fp, "\tdeflateEnd(&s);\n\n");
	fprintf(c_fp, "\tv->siz = s.total_out;\n");
	fprintf(c_fp, "\tsend_u32(p, false, v->siz);\n");
	fprintf(c_fp, "\tsend_u32(p, false, v->blob->siz);\n");
	fprintf(c_fp, "\tdragonnet_send_raw(p, submit, compr, v->siz);\n");
	fprintf(c_fp, "}\n\n");
}

static void gen_deserializers(FILE *c_fp)
{
	for (u8 bits = 8; bits <= 64; bits *= 2) {
		char *fmt = FUNC "void recv_n%d(DragonnetPeer *p, void *buf)\n";

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
			char *fmt = FUNC "void recv_v%dn%d(DragonnetPeer *p, void *buf)\n";

			fprintf(c_fp, fmt, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i)\n", elems);
			fprintf(c_fp, "\t\trecv_n%d(p, buf);\n", bits);
			fprintf(c_fp, "}\n\n");
		}
	}

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt = FUNC "void recv_aabb%dn%d(DragonnetPeer *p, void *buf)\n";

			fprintf(c_fp, fmt, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i)\n");
			fprintf(c_fp, "\t\trecv_v%dn%d(p, buf);\n", elems, bits);
			fprintf(c_fp, "}\n\n");
		}
	}

	fprintf(c_fp, FUNC "void recv_string(DragonnetPeer *p, void *buf)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tstring v = malloc(1 + (1 << 16));\n\n");
	fprintf(c_fp, "\tchar ch;\n");
	fprintf(c_fp, "\tfor (u16 i = 0; ch != '\\0'; ++i) {\n");
	fprintf(c_fp, "\t\trecv_n8(p, &ch);\n");
	fprintf(c_fp, "\t\tv[i] = ch;\n");
	fprintf(c_fp, "\t}\n\n");
	fprintf(c_fp, "\tv = realloc(v, strlen(v));\n");
	fprintf(c_fp, "\tmemcpy(buf, v, strlen(v));\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, FUNC "void recv_Blob(DragonnetPeer *p, void *buf)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tBlob v = (Blob) buf;\n");
	fprintf(c_fp, "\trecv_n32(p, &v->siz);\n");
	fprintf(c_fp, "\tv->data = malloc(v->siz);\n");
	fprintf(c_fp, "\tdragonnet_recv_raw(p, v->data, v->siz);\n\n");
	fprintf(c_fp, "\tmemcpy(buf, v, v->siz + sizeof v);\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, FUNC "void recv_CompressedBlob(DragonnetPeer *p, void *buf)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tCompressedBlob v = *(CompressedBlob *) buf;\n");
	fprintf(c_fp, "\tv->blob = malloc(sizeof *v->blob);\n\n");
	fprintf(c_fp, "\trecv_n32(p, &v->siz);\n");
	fprintf(c_fp, "\trecv_n32(p, &v->blob->siz);\n");
	fprintf(c_fp, "\tv->blob->data = malloc(v->blob->siz);\n\n");
	fprintf(c_fp, "\tchar compr[v->siz];\n");
	fprintf(c_fp, "\tdragonnet_recv_raw(p, compr, sizeof compr);\n\n");
	fprintf(c_fp, "\tz_stream s;\n");
	fprintf(c_fp, "\ts.zalloc = Z_NULL;\n");
	fprintf(c_fp, "\ts.zfree = Z_NULL;\n");
	fprintf(c_fp, "\ts.opaque = Z_NULL;\n\n");
	fprintf(c_fp, "\ts.avail_in = v->siz;\n");
	fprintf(c_fp, "\ts.next_in = (Bytef *) compr;\n");
	fprintf(c_fp, "\ts.avail_out = v->blob->siz;\n");
	fprintf(c_fp, "\ts.next_out = (Bytef *) v->blob->data;\n\n");
	fprintf(c_fp, "\tinflateInit(&s);\n");
	fprintf(c_fp, "\tinflate(&s, Z_NO_FLUSH);\n");
	fprintf(c_fp, "\tinflateEnd(&s);\n");
	fprintf(c_fp, "}\n\n");
}

// Buffer based

static void gen_buffer_serializers(FILE *c_fp)
{
	for (u8 bits = 8; bits <= 64; bits *= 2) {
		char *fmt_u = FUNC "void buf_write_u%d(Blob blob, u%d v)\n";
		char *fmt_s = FUNC "void buf_write_s%d(Blob blob, s%d v)\n";

		fprintf(c_fp, fmt_u, bits, bits);
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\tu%d be = htobe%d(v);\n", bits, bits);
		fprintf(c_fp, "\traw_buf_write(blob, &be, sizeof be);\n");
		fprintf(c_fp, "}\n\n");

		fprintf(c_fp, fmt_s, bits, bits, "");
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\tbuf_write_u%d(blob, (u%d) v);\n", bits, bits);
		fprintf(c_fp, "}\n\n");

		if (bits >= 32) {
			char *fmt_f = FUNC "void buf_write_f%d(Blob blob, f%d v)\n";

			fprintf(c_fp, fmt_f, bits, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tbuf_write_u%d(blob, (u%d) v);\n", bits, bits);
			fprintf(c_fp, "}\n\n");
		}
	}

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = FUNC "void buf_write_v%du%d(Blob blob, v%du%d v)\n";
			char *fmt_s = FUNC "void buf_write_v%ds%d(Blob blob, v%ds%d v)\n";

			fprintf(c_fp, fmt_u, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tu%d *ptr = &v.x;\n", bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
			fprintf(c_fp, "\t\tbuf_write_u%d(blob, *ptr++);\n", bits);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			fprintf(c_fp, fmt_s, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\ts%d *ptr = &v.x;\n", bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
			fprintf(c_fp, "\t\tbuf_write_s%d(blob, *ptr++);\n", bits);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			if (bits >= 32) {
				char *fmt_f = FUNC "void buf_write_v%df%d(Blob blob, v%df%d v)\n";

				fprintf(c_fp, fmt_f, elems, bits, elems, bits);
				fprintf(c_fp, "{\n");
				fprintf(c_fp, "\tf%d *ptr = &v.x;\n", bits);
				fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
				fprintf(c_fp, "\t\tbuf_write_f%d(blob, *ptr++);\n", bits);
				fprintf(c_fp, "\t}\n");
				fprintf(c_fp, "}\n\n");
			}
		}
	}

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = FUNC "void buf_write_aabb%du%d(Blob blob, aabb%du%d v)\n";
			char *fmt_s = FUNC "void buf_write_aabb%ds%d(Blob blob, aabb%ds%d v)\n";

			fprintf(c_fp, fmt_u, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tv%du%d *ptr = &v.min;\n", elems, bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
			fprintf(c_fp, "\t\tbuf_write_v%du%d(blob, *ptr++);\n", elems, bits);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			fprintf(c_fp, fmt_s, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tv%ds%d *ptr = &v.min;\n", elems, bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
			fprintf(c_fp, "\t\tbuf_write_v%ds%d(blob, *ptr++);\n", elems, bits);
			fprintf(c_fp, "\t}\n");
			fprintf(c_fp, "}\n\n");

			if (bits >= 32) {
				char *fmt_f = FUNC "void buf_write_aabb%df%d(Blob blob, aabb%df%d v)\n";

				fprintf(c_fp, fmt_f, elems, bits, elems, bits);
				fprintf(c_fp, "{\n");
				fprintf(c_fp, "\tv%df%d *ptr = &v.min;\n", elems, bits);
				fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
				fprintf(c_fp, "\t\tbuf_write_v%df%d(blob, *ptr++);\n", elems, bits);
				fprintf(c_fp, "\t}\n");
				fprintf(c_fp, "}\n\n");
			}
		}
	}

	fprintf(c_fp, FUNC "void buf_write_string(Blob blob, string v)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\traw_buf_write(blob, v, strlen(v));\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, FUNC "void buf_write_Blob(Blob blob, Blob v)\n\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tbuf_write_u32(blob, v->siz);\n");
	fprintf(c_fp, "\traw_buf_write(blob, v->data, v->siz);\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, FUNC "void buf_write_CompressedBlob(Blob blob, CompressedBlob v)\n\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tchar compr[2 + v->blob->siz];\n\n");
	fprintf(c_fp, "\tz_stream s;\n");
	fprintf(c_fp, "\ts.zalloc = Z_NULL;\n");
	fprintf(c_fp , "\ts.zfree = Z_NULL;\n");
	fprintf(c_fp, "\ts.opaque = Z_NULL;\n\n");
	fprintf(c_fp, "\ts.avail_in = v->blob->siz;\n");
	fprintf(c_fp, "\ts.avail_out = 3 + v->blob->siz;\n");
	fprintf(c_fp, "\ts.next_in = (Bytef *) v->blob->data;\n");
	fprintf(c_fp, "\ts.next_out = (Bytef *) compr;\n\n");
	fprintf(c_fp, "\tdeflateInit(&s, Z_BEST_COMPRESSION);\n");
	fprintf(c_fp, "\tdeflate(&s, Z_FINISH);\n");
	fprintf(c_fp, "\tdeflateEnd(&s);\n\n");
	fprintf(c_fp, "\tv->siz = s.total_out;\n");
	fprintf(c_fp, "\tbuf_write_u32(blob, v->siz);\n");
	fprintf(c_fp, "\tbuf_write_u32(blob, v->blob->siz);\n");
	fprintf(c_fp, "\traw_buf_write(blob, compr, v->siz);\n");
	fprintf(c_fp, "}\n\n");
}

static void gen_buffer_deserializers(FILE *c_fp)
{
	for (u8 bits = 8; bits <= 64; bits *= 2) {
		char *fmt_u = FUNC "u%d buf_read_u%d(Blob blob)\n";
		char *fmt_s = FUNC "s%d buf_read_s%d(Blob blob)\n";

		fprintf(c_fp, fmt_u, bits, bits);
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\tu%d be;\n", bits);
		fprintf(c_fp, "\traw_buf_read(blob, &be, sizeof be);\n");
		fprintf(c_fp, "\treturn be%dtoh(be);\n", bits);
		fprintf(c_fp, "}\n\n");

		fprintf(c_fp, fmt_s, bits, bits);
		fprintf(c_fp, "{\n");
		fprintf(c_fp, "\treturn (s%d) buf_read_u%d(blob);\n", bits, bits);
		fprintf(c_fp, "}\n\n");

		if (bits >= 32) {
			char *fmt_f = FUNC "f%d buf_read_f%d(Blob blob)\n";

			fprintf(c_fp, fmt_f, bits, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\treturn (f%d) buf_read_u%d(blob);\n", bits, bits);
			fprintf(c_fp, "}\n\n");
		}
	}

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = FUNC "v%du%d buf_read_v%du%d(Blob blob)\n";
			char *fmt_s = FUNC "v%ds%d buf_read_v%ds%d(Blob blob)\n";

			fprintf(c_fp, fmt_u, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tv%du%d v = {0};\n", elems, bits);
			fprintf(c_fp, "\tu%d *ptr = &v.x;\n\n", bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
			fprintf(c_fp, "\t\t*ptr++ = buf_read_u%d(blob);\n", bits);
			fprintf(c_fp, "\t}\n\n");
			fprintf(c_fp, "\treturn v;\n");
			fprintf(c_fp, "}\n\n");

			fprintf(c_fp, fmt_s, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\tv%ds%d v = {0};\n", elems, bits);
			fprintf(c_fp, "\ts%d *ptr = &v.x;\n\n", bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
			fprintf(c_fp, "\t\t*ptr++ = buf_read_s%d(blob);\n", bits);
			fprintf(c_fp, "\t}\n\n");
			fprintf(c_fp, "\treturn v;\n");
			fprintf(c_fp, "}\n\n");

			if (bits >= 32) {
				char *fmt_f = FUNC "v%df%d buf_read_v%df%d(Blob blob)\n";

				fprintf(c_fp, fmt_f, elems, bits, elems, bits);
				fprintf(c_fp, "{\n");
				fprintf(c_fp, "\tv%df%d v = {0};\n", elems, bits);
				fprintf(c_fp, "\tf%d *ptr = &v.x;\n\n", bits);
				fprintf(c_fp, "\tfor (u8 i = 0; i < %d; ++i) {\n", elems);
				fprintf(c_fp, "\t\t*ptr++ = buf_read_f%d(blob);\n", bits);
				fprintf(c_fp, "\t}\n\n");
				fprintf(c_fp, "\treturn v;\n");
				fprintf(c_fp, "}\n\n");
			}
		}
	}

	for (u8 elems = 2; elems <= 4; ++elems) {
		for (u8 bits = 8; bits <= 64; bits *= 2) {
			char *fmt_u = FUNC "aabb%du%d buf_read_aabb%du%d(Blob blob)\n";
			char *fmt_s = FUNC "aabb%ds%d buf_read_aabb%ds%d(Blob blob)\n";

			fprintf(c_fp, fmt_u, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\taabb%du%d v = {0};\n", elems, bits);
			fprintf(c_fp, "\tv%du%d *ptr = &v.min;\n\n", elems, bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
			fprintf(c_fp, "\t\t*ptr++ = buf_read_v%du%d(blob);\n", elems, bits);
			fprintf(c_fp, "\t}\n\n");
			fprintf(c_fp, "\treturn v;\n");
			fprintf(c_fp, "}\n\n");

			fprintf(c_fp, fmt_s, elems, bits, elems, bits);
			fprintf(c_fp, "{\n");
			fprintf(c_fp, "\taabb%ds%d v = {0};\n", elems, bits);
			fprintf(c_fp, "\tv%ds%d *ptr = &v.min;\n\n", elems, bits);
			fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
			fprintf(c_fp, "\t\t*ptr++ = buf_read_v%ds%d(blob);\n", elems, bits);
			fprintf(c_fp, "\t}\n\n");
			fprintf(c_fp, "\treturn v;\n");
			fprintf(c_fp, "}\n\n");

			if (bits >= 32) {
				char *fmt_f = FUNC "aabb%df%d buf_read_aabb%df%d(Blob blob)\n";

				fprintf(c_fp, fmt_f, elems, bits, elems, bits);
				fprintf(c_fp, "{\n");
				fprintf(c_fp, "\taabb%df%d v = {0};\n", elems, bits);
				fprintf(c_fp, "\tv%df%d *ptr = &v.min;\n\n", elems, bits);
				fprintf(c_fp, "\tfor (u8 i = 0; i < 2; ++i) {\n");
				fprintf(c_fp, "\t\t*ptr++ = buf_read_v%df%d(blob);\n", elems, bits);
				fprintf(c_fp, "\t}\n\n");
				fprintf(c_fp, "\treturn v;\n");
				fprintf(c_fp, "}\n\n");
			}
		}
	}

	fprintf(c_fp, FUNC "string buf_read_string(Blob blob)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tstring v = malloc(sizeof(u16));\n\n");
	fprintf(c_fp, "\tchar ch;\n");
	fprintf(c_fp, "\tfor (u16 i = 0; ch != '\\0'; ++i) {\n");
	fprintf(c_fp, "\t\tch = buf_read_s8(blob);\n");
	fprintf(c_fp, "\t\tv[i] = ch;\n");
	fprintf(c_fp, "\t}\n\n");
	fprintf(c_fp, "\tv = realloc(v, strlen(v));\n");
	fprintf(c_fp, "\treturn v;\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, FUNC "Blob buf_read_Blob(Blob blob)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tBlob v = malloc(sizeof *v);\n");
	fprintf(c_fp, "\tv->siz = buf_read_u32(blob);\n");
	fprintf(c_fp, "\tv->data = malloc(v->siz);\n");
	fprintf(c_fp, "\traw_buf_read(blob, v->data, v->siz);\n\n");
	fprintf(c_fp, "\treturn v;\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, FUNC "CompressedBlob buf_read_CompressedBlob(Blob blob)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tCompressedBlob v = malloc(sizeof *v);\n");
	fprintf(c_fp, "\tv->blob = malloc(sizeof *v->blob);\n\n");
	fprintf(c_fp, "\tv->siz = buf_read_u32(blob);\n");
	fprintf(c_fp, "\tv->blob->siz = buf_read_u32(blob);\n");
	fprintf(c_fp, "\tv->blob->data = malloc(v->blob->siz);\n\n");
	fprintf(c_fp, "\tchar compr[v->siz];\n");
	fprintf(c_fp, "\traw_buf_read(blob, compr, sizeof compr);\n\n");
	fprintf(c_fp, "\tz_stream s;\n");
	fprintf(c_fp, "\ts.zalloc = Z_NULL;\n");
	fprintf(c_fp, "\ts.zfree = Z_NULL;\n");
	fprintf(c_fp, "\ts.opaque = Z_NULL;\n\n");
	fprintf(c_fp, "\ts.avail_in = v->siz;\n");
	fprintf(c_fp, "\ts.next_in = (Bytef *) compr;\n");
	fprintf(c_fp, "\ts.avail_out = v->blob->siz;\n");
	fprintf(c_fp, "\ts.next_out = (Bytef *) v->blob->data;\n\n");
	fprintf(c_fp, "\tinflateInit(&s);\n");
	fprintf(c_fp, "\tinflate(&s, Z_NO_FLUSH);\n");
	fprintf(c_fp, "\tinflateEnd(&s);\n\n");
	fprintf(c_fp, "\treturn v;\n");
	fprintf(c_fp, "}\n\n");
}

static void gen_deallocators(FILE *c_fp)
{
	for (u8 bits = 8; bits <= 64; bits *= 2) {
		fprintf(c_fp, FUNC "void free_u%d(__attribute__((unused)) u%d v) {}\n", bits, bits);
		fprintf(c_fp, FUNC "void free_s%d(__attribute__((unused)) s%d v) {}\n", bits, bits);
		if (bits >= 32)
			fprintf(c_fp, FUNC "void free_f%d(__attribute__((unused)) f%d v) {}\n", bits, bits);

		for (u8 elems = 2; elems <= 4; ++elems) {
			fprintf(c_fp, FUNC "void free_v%du%d(__attribute__((unused)) v%du%d v) {}\n", elems, bits, elems, bits);
			fprintf(c_fp, FUNC "void free_v%ds%d(__attribute__((unused)) v%ds%d v) {}\n", elems, bits, elems, bits);
			if (bits >= 32)
				fprintf(c_fp, FUNC "void free_v%df%d(__attribute__((unused)) v%df%d v) {}\n", elems, bits, elems, bits);

			fprintf(c_fp, FUNC "void free_aabb%du%d(__attribute__((unused)) aabb%du%d v) {}\n", elems, bits, elems, bits);
			fprintf(c_fp, FUNC "void free_aabb%ds%d(__attribute__((unused)) aabb%ds%d v) {}\n", elems, bits, elems, bits);
			if (bits >= 32)
				fprintf(c_fp, FUNC "void free_aabb%df%d(__attribute__((unused)) aabb%df%d v) {}\n", elems, bits, elems, bits);
		}
	}

	fprintf(c_fp, FUNC "\nvoid free_string(string str)\n{\n\tfree(str);\n}\n\n");
	fprintf(c_fp, FUNC "void free_Blob(Blob blob)\n{\n\tdragonnet_free_Blob(blob);\n}\n\n");
	fprintf(c_fp, FUNC "void free_CompressedBlob(CompressedBlob comp)\n{\n\tdragonnet_free_CompressedBlob(comp);\n}\n\n");
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
	fprintf(c_fp, "/*\n");
	fprintf(c_fp, "\tThis file was automatically generated by Dragonnet.\n");
	fprintf(c_fp, "\tDo NOT edit it manually. Instead, edit types.dnet and re-run DragonnetTypegen.\n");
	fprintf(c_fp, "*/\n\n");
	fprintf(c_fp, "#include <dragonnet/recv.h>\n");
	fprintf(c_fp, "#include <dragonnet/send.h>\n");
	fprintf(c_fp, "#include <endian.h/endian.h>\n");
	fprintf(c_fp, "#include <errno.h>\n");
	fprintf(c_fp, "#include <stdbool.h>\n");
	fprintf(c_fp, "#include <stdio.h>\n");
	fprintf(c_fp, "#include <stdlib.h>\n");
	fprintf(c_fp, "#include <string.h>\n");
	fprintf(c_fp, "#include <zlib.h>\n\n");

	fprintf(c_fp, "#define htobe8(x) (x)\n");
	fprintf(c_fp, "#define be8toh(x) (x)\n\n");
	fprintf(c_fp, "#include \"dnet-types.h\"\n\n");

	FILE *h_fp = fopen("dnet-types.h", "w");
	fprintf(h_fp, "/*\n");
	fprintf(h_fp, "\tThis file was automatically generated by Dragonnet.\n");
	fprintf(h_fp, "\tDo NOT edit it manually. Instead, edit types.dnet and re-run DragonnetTypegen.\n");
	fprintf(h_fp, "*/\n\n");
	fprintf(h_fp, "#ifndef _DRAGONNET_TYPES_H_\n");
	fprintf(h_fp, "#define _DRAGONNET_TYPES_H_\n");
	fprintf(h_fp, "#include <dragonnet/peer.h>\n");
	fprintf(h_fp, "#include <dragontype/number.h>\n");
	fprintf(h_fp, "#include <stddef.h>\n");
	fprintf(h_fp, "typedef char *string;\n\n");
	fprintf(h_fp, "typedef struct {\n\tu32 siz;\n\tu8 *data;\n} *Blob;\n\n");
	fprintf(h_fp, "typedef struct {\n");
	fprintf(h_fp, "\tu32 siz;\n");
	fprintf(h_fp, "\tBlob blob;\n");
	fprintf(h_fp, "} *CompressedBlob;\n\n");

	// buffer allocation
	fprintf(h_fp, "Blob dragonnet_create_Blob();\n");
	fprintf(h_fp, "void dragonnet_free_Blob(Blob blob);\n\n");
	fprintf(h_fp, "CompressedBlob dragonnet_create_CompressedBlob();\n");
	fprintf(h_fp, "void dragonnet_free_CompressedBlob(CompressedBlob comp);\n\n");

	fprintf(c_fp, "Blob dragonnet_create_Blob()\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tBlob blob = malloc(sizeof *blob);\n");
	fprintf(c_fp, "\tblob->data = NULL;\n");
	fprintf(c_fp, "\tblob->siz = 0;\n");
	fprintf(c_fp, "\treturn blob;\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, "void dragonnet_free_Blob(Blob blob)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tif (blob->data)\n");
	fprintf(c_fp, "\t\tfree(blob->data);\n");
	fprintf(c_fp, "\tfree(blob);\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, "CompressedBlob dragonnet_create_CompressedBlob()\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tCompressedBlob comp = malloc(sizeof *comp);\n");
	fprintf(c_fp, "\tcomp->blob = dragonnet_create_Blob();\n");
	fprintf(c_fp, "\tcomp->siz = 0;\n");
	fprintf(c_fp, "\treturn comp;\n");
	fprintf(c_fp, "}\n\n");

	fprintf(c_fp, "void dragonnet_free_CompressedBlob(CompressedBlob comp)\n");
	fprintf(c_fp, "{\n");
	fprintf(c_fp, "\tdragonnet_free_Blob(comp->blob);\n");
	fprintf(c_fp, "\tfree(comp);\n");
	fprintf(c_fp, "}\n\n");

	gen_serializers(c_fp);
	gen_deserializers(c_fp);

	gen_buffer_serializers(c_fp);
	gen_buffer_deserializers(c_fp);

	gen_deallocators(c_fp);

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
			fprintf(c_fp, FUNC "void send_%s(DragonnetPeer *p, bool submit, %s type)\n{\n", msg, msg);
		} else {
			char **tokens;
			size_t tokens_len = split(&tokens, msgs[i], " ");

			char *arr = process_array(c_fp, tokens[1]);
			fprintf(c_fp, "send_%s(p, %s, type.%s%s);\n", &tokens[0][1], (i == msgs_len - 1 || msgs[i + 1][0] != '\t') ? "submit" : "false", tokens[1], arr);
			free(arr);

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

			char *arr = process_array(c_fp, tokens[1]);
			if (i >= msgs_len-1 || msgs[1+i][0] != '\t')
				fprintf(c_fp, "send_%s(p, true, type->%s%s);\n", &tokens[0][1], tokens[1], arr);
			else
				fprintf(c_fp, "send_%s(p, false, type->%s%s);\n", &tokens[0][1], tokens[1], arr);
			free(arr);

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
			fprintf(c_fp, FUNC "void recv_%s(DragonnetPeer *p, void *buf)\n{\n", msg);
			fprintf(c_fp, "\t%s *type = (%s *) buf;\n", msg, msg);
		} else {
			char **tokens;
			size_t tokens_len = split(&tokens, msgs[i], " ");

			char type[strlen(&tokens[0][1])];
			strcpy(type, &tokens[0][1]);

			for (char *tptr = type; *tptr != '\0'; tptr++) {
				for (size_t bits = 8; bits <= 64; bits *= 2) {
					const char *fmt[] = {"u%d", "s%d", "f%d"};
					for (size_t j = 0; j < sizeof fmt / sizeof *fmt; ++j) {
						char *cmp;
						asprintf(&cmp, fmt[j], bits);
						size_t diff = strcmp(tptr, cmp);
						free(cmp);

						if (diff == 0) {
							sprintf(tptr, "n%ld", bits);
							goto n_done;
						}
					}
				}
			}

			n_done: (void) 0;

			char *arr = process_array(c_fp, tokens[1]);
			fprintf(c_fp, "recv_%s(p, &type->%s%s);\n", type, tokens[1], arr);
			free(arr);

			free_split(tokens, tokens_len);
			tokens = NULL;
		}
	}

	fprintf(c_fp, "}\n\n");
	msg = NULL;

	// Buffer (de)serialization
	for (size_t i = 0; i < msgs_len; ++i) {
		if (msgs[i][0] != '\t') {
			if (msg != NULL)
				fprintf(c_fp, "}\n\n");

			msg = msgs[i];
			fprintf(h_fp, "void dragonnet_buf_write_%s(Blob blob, %s type);", msg, msg);
			fprintf(c_fp, "void buf_write_%s(Blob blob, %s type)\n{\n", msg, msg);
			fprintf(c_fp, "\ndragonnet_buf_write_%s(blob, type);\n}\n\n", msg);
			fprintf(c_fp, "void dragonnet_buf_write_%s(Blob blob, %s type)\n{\n", msg, msg);
		} else {
			char **tokens;
			size_t tokens_len = split(&tokens, msgs[i], " ");

			char *arr = process_array(c_fp, tokens[1]);
			fprintf(c_fp, "buf_write_%s(blob, type.%s%s);\n", &tokens[0][1], tokens[1], arr);
			free(arr);

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
			fprintf(h_fp, "%s dragonnet_buf_read_%s(Blob blob);\n", msg, msg);
			fprintf(c_fp, FUNC "%s buf_read_%s(Blob blob){\n", msg, msg);
			fprintf(c_fp, "\treturn dragonnet_buf_read_%s(blob);\n}\n\n", msg);
			fprintf(c_fp, "%s dragonnet_buf_read_%s(Blob blob)\n{\n", msg, msg);
			fprintf(c_fp, "\t%s type = {0};\n", msg);
		} else {
			char **tokens;
			size_t tokens_len = split(&tokens, msgs[i], " ");

			char *arr = process_array(c_fp, tokens[1]);
			fprintf(c_fp, "type.%s%s = buf_read_%s(blob);\n", tokens[1], arr, &tokens[0][1]);
			free(arr);

			free_split(tokens, tokens_len);
			tokens = NULL;
		}
	}

	fprintf(h_fp, "\n");
	fprintf(c_fp, "\treturn type;\n");
	fprintf(c_fp, "}\n\n");
	msg = NULL;

	// Deallocators
	for (size_t i = 0; i < msgs_len; ++i) {
		if (msgs[i][0] != '\t') {
			if (msg != NULL)
				fprintf(c_fp, "}\n\n");

			msg = msgs[i];
			fprintf(h_fp, "void dragonnet_free_%s(%s type);", msg, msg);
			fprintf(c_fp, "void free_%s(%s type)\n{\n", msg, msg);
			fprintf(c_fp, "\tdragonnet_free_%s(type);\n}\n\n", msg);
			fprintf(c_fp, "void dragonnet_free_%s(%s type)\n{\n", msg, msg);
		} else {
			char **tokens;
			size_t tokens_len = split(&tokens, msgs[i], " ");

			char *arr = process_array(c_fp, tokens[1]);
			fprintf(c_fp, "free_%s(type.%s%s);\n", &tokens[0][1], tokens[1], arr);
			free(arr);

			free_split(tokens, tokens_len);
			tokens = NULL;
		}
	}

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
	fprintf(h_fp, "\n#endif\n");

	// ABI
	fprintf(c_fp, "u16 dragonnet_num_types = DRAGONNET_NUM_TYPES;\n");
	fprintf(c_fp, "DragonnetType dragonnet_types[] = {\n");

	for (size_t i = 0; i < msgs_len; ++i) {
		if (msgs[i][0] == '\t')
			continue;

		fprintf(c_fp, "\t{\n");
		fprintf(c_fp, "\t\t.siz = sizeof(%s),\n", msgs[i]);
		fprintf(c_fp, "\t\t.deserialize = &recv_%s\n", msgs[i]);
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
