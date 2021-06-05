#ifndef PROTON_HEADER
#define PROTON_HEADER


#ifndef _Nullable
#define _Nullable
#endif

#ifdef __cplusplus
extern "C"{
#endif 

int prtn_initialize(void);
int prtn_set_title (const char *  _Nullable title);
int prtn_set_menu_extra_text (const char * _Nullable text);
int prtn_add_menu_extra_item (const char * _Nullable text);
int prtn_add_content_path (const char * _Nullable path);
int prtn_add_script_message_handler(const char * _Nullable name);
int prtn_execute_script(const char * _Nullable script);


int xmain (void);

#ifdef __cplusplus
}
#endif 

// #ifndef GO_CGO_EXPORT_PROLOGUE_H
// struct prtn_goTrampoline_return {
// 	char* _Nullable r0;
// 	char* _Nullable r1;
// };
// extern struct prtn_goTrampoline_return prtn_goTrampoline(char* _Nullable param1, char* _Nullable param2);
// #endif 

#endif
