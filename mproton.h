#ifndef CGOEXP_HEADER
#define CGOEXP_HEADER


#ifndef _Nullable
#define _Nullable
#endif

int initialize(void);
int set_title (const char *  _Nullable title);
int set_menu_extra_text (const char * _Nullable text);
int add_menu_extra_item (const char * _Nullable text);
int add_content_path (const char * _Nullable path);
int add_script_message_handler(const char * _Nullable name);


int xmain (void);

#ifndef GO_CGO_EXPORT_PROLOGUE_H
struct goTrampoline_return {
	char* _Nullable r0;
	char* _Nullable r1;
};
extern struct goTrampoline_return goTrampoline(long long a, char* _Nullable param1, char* _Nullable param2);
#endif 

#endif
