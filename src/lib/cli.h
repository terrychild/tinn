#ifndef LIB_ARGS_H
#define LIB_ARGS_H

bool cliArg(int argc, char* argv[], const char* name);
char* cliValue(int argc, char* argv[], const char* name, char* default_value);

#endif