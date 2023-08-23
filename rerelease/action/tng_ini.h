#define MAX_INI_STR_LEN 128
#define MAX_INI_SIZE 1024L

char *INI_Find(FILE *fh, const char *section, const char *key);

typedef struct
{
	FILE *pfile;
	qboolean inparsing;
	qboolean nextline;
	char cline[1024];
	char ctoken[1024];
	int lnumber;
}
parse_t;


// ini functions
#define ini_t parse_t
#define OpenIniFile(a,b) ParseStartFile(a,b)
#define CloseIniFile(a) ParseEndFile(a)
#define INI_STR_LEN 128
typedef char inistr[INI_STR_LEN];

char *IniPath(void);
int ReadIniSection(ini_t *ini, char *section, char buf[][INI_STR_LEN], int maxarraylen);
char *ReadIniStr(ini_t *ini, char *section, char *key, char *value, char *defvalue);
int ReadIniInt(ini_t *ini, char *section, char *key, int defvalue);

// parse functions
qboolean ParseStartFile(char *filename, parse_t *parse);
void ParseEndFile(parse_t *parse);
char *ParseNextToken(parse_t *parse, char *seperator);
