#include "g_local.h"

#define MAX_INI_STR_LEN 128
#define MAX_INI_SIZE 1024L

char *INI_Find( FILE *fh, const char *section, const char *key )
{
	char _ini_file[MAX_INI_SIZE] = "";
	static char _ini_ret[MAX_INI_STR_LEN] = { 0 };
	char *line = NULL, *value = NULL;
	char cur_section[MAX_INI_STR_LEN] = { 0 };
	size_t length = 0;
	int was_at = 0, read_size = 0;

	if( ! fh )
	{
		gi.dprintf( "INI_Find: file handle was NULL\n" );
		return NULL;
	}

	memset( _ini_ret, 0, MAX_INI_STR_LEN );
	memset( _ini_file, 0, MAX_INI_SIZE );

	was_at = fseek( fh, 0, SEEK_CUR );
	fseek( fh, 0, SEEK_SET );
	read_size = fread( _ini_file, 1, MAX_INI_SIZE, fh );
	fseek( fh, was_at, SEEK_SET );

	if( read_size == MAX_INI_SIZE )
	{
		_ini_file[ MAX_INI_SIZE - 1 ] = '\0';
		gi.dprintf( "INI_Find: file is too long, ignoring the end\n" );
	}

	// This allows us to handle pre-section name/value pairs without redundant code below.
	if( ! section )
		section = "";

	line = strtok( _ini_file, "\r\n" );
	while( line )
	{
		length = strlen(line);
		if( (length > 2) && (line[0] != ';') )
		{
			if( (line[0] == '[') && (line[length-1] == ']') )
			{
				length --;
				if( length > MAX_INI_STR_LEN )
					length = MAX_INI_STR_LEN;
				Q_strncpyz( cur_section, line+1, length );
			}
			else
			{
				value = strchr( line+1, '=' );
				if( ! value )
					gi.dprintf( "INI_Find: invalid key/value pair: \"%s\"\n", line );
				else
				{
					*value = '\0'; // Null the delimeter; now line points to key and value+1 to value.
					value ++;

					if( (Q_stricmp( section, cur_section ) == 0) && (Q_stricmp( line, key ) == 0) )
					{
						Q_strncpyz( _ini_ret, value, MAX_INI_STR_LEN );
						return _ini_ret;
					}
				}
			}
		}
		
		line = strtok( NULL, "\r\n" );
	}

	return NULL;
}



#ifndef MAX_STR_LEN
#define MAX_STR_LEN 1000
#endif				// MAX_STR_LEN

static char _inipath[MAX_STR_LEN] = "";

static char *StripSpaces(char *astring)
{
	char *p, buf[1024];

	p = astring;
	if (!p || !*p)
		return astring;

	while (*p != '\0' && (*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t'))
		p++;
	if (!*p) {
		*astring = '\0';
		return astring;
	}

	strcpy(buf, p);
	p = &buf[strlen(buf) - 1];

	while (p != buf && (*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t'))
		p--;
	p++;
	*p = '\0';
	strcpy(astring, buf);
	return astring;
}

//
static qboolean CheckForRemark(char *src)
{
	char *myptr;

	myptr = StripSpaces(src);
	if (!myptr || !*myptr)
		return (true);
	if (*myptr == '#')
		return (true);
	if (strlen(myptr) > 1 && *myptr == '/') {
		myptr++;
		return (*myptr == '/');
	}

	return false;
}

//
qboolean ParseStartFile(char *filename, parse_t * parse)
{
	if (parse->inparsing == true)
		return false;
	parse->pfile = fopen(filename, "r");
	if (parse->pfile) {
		parse->inparsing = true;
		parse->nextline = true;
		parse->lnumber = 0;
		return true;
	}
	else
		gi.dprintf("Error opening file %s\n", filename);
	return false;
}

//
void ParseEndFile(parse_t * parse)
{
	if (parse->pfile) {
		fclose(parse->pfile);
		parse->pfile = NULL;
		parse->inparsing = false;
		parse->nextline = false;
		parse->lnumber = 0;
	}
}

//due the usage of strtok, only ONE file should be parsed at
//the same time...this may change in the future.
char *ParseNextToken(parse_t * parse, char *seperator)
{
	char *dummy;

	if (parse->inparsing == true) {
		dummy = NULL;
		while (!dummy) {
			if (parse->nextline == true) {
				do {
					dummy = fgets(parse->cline, 1020, parse->pfile);
					parse->lnumber++;
					if (!dummy)	//we reached end of file
						return NULL;
				} while (CheckForRemark(parse->cline));
				dummy = strtok(parse->cline, seperator);
				if (dummy)	//this should always happen :)
					parse->nextline = false;
			}
			else {
				dummy = strtok(NULL, seperator);
				if (!dummy)	//no more tokens in current line
					parse->nextline = true;
			}
		}
		//we finally got a new token
		strcpy(parse->ctoken, dummy);
		StripSpaces(parse->ctoken);
		return (parse->ctoken);
	}
	return NULL;
}

// internal use only, ini section must occupy one line
// and has the format "[section]". This may be altered
// Sections are case insensitive.
static qboolean _seekinisection(FILE * afile, char *section)
{
	inistr buf, comp;

	if (!afile)
		return false;
	sprintf(comp, "[%s]", section);
	fseek(afile, 0, SEEK_SET);
	while (fgets(buf, INI_STR_LEN, afile)) {
		if (Q_stricmp(StripSpaces(buf), comp) == 0)
			return true;
	}
	return false;
}

// internal use only, make sure sizeof(value) >= INI_STR_LEN
static char *_readinivalue(FILE * afile, char *section, char *key, char *value)
{
	inistr buf, akey;
	char *p;

	if (!afile)
		return NULL;

	if (_seekinisection(afile, section) == true) {
		while (fgets(buf, INI_STR_LEN, afile)) {
			//splitting line into key and value
			p = strrchr(buf, '=');
			if (p) {
				*p = '\0';
				p++;
				strcpy(akey, buf);
				strcpy(value, p);
			}
			else {
				strcpy(akey, buf);
				value[0] = '\0';
			}
			if (Q_stricmp(StripSpaces(akey), key) == 0)
				return (StripSpaces(value));	// found!
			if (akey[0] == '[')
				return NULL;	// another section begins
		}
	}
	return NULL;
}

int ReadIniSection(ini_t * ini, char *section, char buf[][INI_STR_LEN], int maxarraylen)
{
	int i;
	char *p;

	i = 0;
	if (_seekinisection(ini->pfile, section) == true) {
		while (i < maxarraylen && fgets(buf[i], 127, ini->pfile)) {
			buf[i][127] = '\0';
			p = buf[i];
			StripSpaces(p);
			if (*p == '[')	// next section?
			{
				*p = '\0';
				return i;
			}
			i++;
		}
	}
	return i;
}

char *ReadIniStr(ini_t * ini, char *section, char *key, char *value, char *defvalue)
{
	if (!_readinivalue(ini->pfile, section, key, value))
		strcpy(value, defvalue);
	return value;
}

int ReadIniInt(ini_t * ini, char *section, char *key, int defvalue)
{
	inistr value;

	if (_readinivalue(ini->pfile, section, key, value))
		return atoi(value);
	return defvalue;
}

char *IniPath(void)
{
	cvar_t *p;

	if (!*_inipath) {
		p = gi.cvar("ininame", "action.ini", 0);
		if (p->string && *(p->string))
			sprintf(_inipath, "%s/%s", GAMEVERSION, p->string);
		else
			sprintf(_inipath, "%s/%s", GAMEVERSION, "action.ini");
	}
	return _inipath;
}
