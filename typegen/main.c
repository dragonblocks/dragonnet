#include <dragontype/number.h>
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

int main(__attribute((unused)) int argc, __attribute((unused)) char **argv)
{
	FILE *fp = fopen("types.dnet", "r");

	char data[1 << 16];
	memset(data, 0, sizeof data);
	fread(data, sizeof *data, sizeof data, fp);

	fclose(fp);
	fp = fopen("dnet-types.h", "w");
	fprintf(fp, "#include <dragontype/number.h>\n\n");

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

			char type[15];
			if (    // Numeric types
					strcmp(&tokens[0][1], "u8") == 0 || strcmp(&tokens[0][1], "s8")
					|| strcmp(&tokens[0][1], "u16") || strcmp(&tokens[0][1], "s16")
					|| strcmp(&tokens[0][1], "u32") || strcmp(&tokens[0][1], "s32")
					|| strcmp(&tokens[0][1], "u64") || strcmp(&tokens[0][1], "s64")
					|| strcmp(&tokens[0][1], "f32") || strcmp(&tokens[0][1], "f64")
					// Vectors (2 elements)
					|| strcmp(&tokens[0][1], "v2u8") || strcmp(&tokens[0][1], "v2s8")
					|| strcmp(&tokens[0][1], "v2u16") || strcmp(&tokens[0][1], "v2s16")
					|| strcmp(&tokens[0][1], "v2u32") || strcmp(&tokens[0][1], "v2s32")
					|| strcmp(&tokens[0][1], "v2u64") || strcmp(&tokens[0][1], "v2s64")
					|| strcmp(&tokens[0][1], "v2f32") || strcmp(&tokens[0][1], "v2f32")
					|| strcmp(&tokens[0][1], "v2f64") || strcmp(&tokens[0][1], "v2f64")
					// Vectors (3 elements)
					|| strcmp(&tokens[0][1], "v3u8") || strcmp(&tokens[0][1], "v3s8")
					|| strcmp(&tokens[0][1], "v3u16") || strcmp(&tokens[0][1], "v3s16")
					|| strcmp(&tokens[0][1], "v3u32") || strcmp(&tokens[0][1], "v3s32")
					|| strcmp(&tokens[0][1], "v3u64") || strcmp(&tokens[0][1], "v3s64")
					|| strcmp(&tokens[0][1], "v3f32") || strcmp(&tokens[0][1], "v3f64")
					// Vectors (4 elements)
					|| strcmp(&tokens[0][1], "v4u8") || strcmp(&tokens[0][1], "v4s8")
					|| strcmp(&tokens[0][1], "v4u16") || strcmp(&tokens[0][1], "v4s16")
					|| strcmp(&tokens[0][1], "v4u32") || strcmp(&tokens[0][1], "v4s32")
					|| strcmp(&tokens[0][1], "v4u64") || strcmp(&tokens[0][1], "v4s64")
					|| strcmp(&tokens[0][1], "v4f32") || strcmp(&tokens[0][1], "v4f64")
					// AABB2
					|| strcmp(&tokens[0][1], "aabb2u8") || strcmp(&tokens[0][1], "aabb2s8")
					|| strcmp(&tokens[0][1], "aabb2u16") || strcmp(&tokens[0][1], "aabb2s16")
					|| strcmp(&tokens[0][1], "aabb2u32") || strcmp(&tokens[0][1], "aabb2s32")
					|| strcmp(&tokens[0][1], "aabb2u64") || strcmp(&tokens[0][1], "aabb2s64")
					|| strcmp(&tokens[0][1], "aabb2f32") || strcmp(&tokens[0][1], "aabb2f64")
					// AABB3
					|| strcmp(&tokens[0][1], "aabb3u8") || strcmp(&tokens[0][1], "aabb3s8")
					|| strcmp(&tokens[0][1], "aabb3u16") || strcmp(&tokens[0][1], "aabb3s16")
					|| strcmp(&tokens[0][1], "aabb3u32") || strcmp(&tokens[0][1], "aabb3s32")
					|| strcmp(&tokens[0][1], "aabb3u64") || strcmp(&tokens[0][1], "aabb3s64")
					|| strcmp(&tokens[0][1], "aabb3f32") || strcmp(&tokens[0][1], "aabb3f64")
					// AABB4
					|| strcmp(&tokens[0][1], "aabb4u8") || strcmp(&tokens[0][1], "aabb4s8")
					|| strcmp(&tokens[0][1], "aabb4u16") || strcmp(&tokens[0][1], "aabb4s16")
					|| strcmp(&tokens[0][1], "aabb4u32") || strcmp(&tokens[0][1], "aabb4s32")
					|| strcmp(&tokens[0][1], "aabb4u64") || strcmp(&tokens[0][1], "aabb4s64")
					|| strcmp(&tokens[0][1], "aabb4f32") || strcmp(&tokens[0][1], "aabb4f64"))
				sprintf(type, "%s ", &tokens[0][1]);
			else if (strcmp(&tokens[0][1], "string"))
				// String
				strcpy(type, "char *");
			else if (strcmp(&tokens[0][1], "blob"))
				// Blob
				strcpy(type, "DragonnetBlob ");

			fprintf(fp, "\t%s%s;\n", type, &tokens[0][1]);
			free(tokens);
		}
	}

	fprintf(fp, "} %s;\n", msg);

	free(msgs);
	fclose(fp);
}
