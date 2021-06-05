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
int prtn_add_menu_extra_item (const char * _Nullable text, int tag);
int prtn_add_content_path (const char * _Nullable path);
int prtn_add_script_message_handler(const char * _Nullable name);
int prtn_execute_script(const char * _Nullable script);
int prtn_event_loop(void);

#ifdef __cplusplus
}
#endif 


#endif
