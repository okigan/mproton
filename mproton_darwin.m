// +build darwin

//  main.m
//  cmini
//
//  Created by Igor Okulist on 5/27/21.
//

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <WebKit/WebKit.h>

#include "mproton.h"
#include "mproton_cgo_exports.h"

@interface AppContext : NSObject <WKScriptMessageHandler>
@property(nonatomic, retain)  NSWindow  * _Nullable window;
@property(nonatomic, retain)  NSMenu  * _Nullable mainMenu;
@property(nonatomic, retain)  NSStatusItem * _Nullable statusItem;
@property(nonatomic, retain)  WKWebView * _Nullable webView;

- (void)userContentController:(nonnull WKUserContentController *)userContentController
      didReceiveScriptMessage:(nonnull WKScriptMessage *)message;

@end

@implementation AppContext

- (void)userContentController:(nonnull WKUserContentController *)userContentController
      didReceiveScriptMessage:(nonnull WKScriptMessage *)message {
    NSLog(@"[objc %@] int WKScriptMessageHandler callback called", [NSThread currentThread]);

    NSDictionary *paramDict = message.body;

    const char *name = [ message.name UTF8String ];

    NSError *writeError = nil; 
    NSData *jsonData = [NSJSONSerialization dataWithJSONObject:message.body options:0 error:&writeError];
    NSString *jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding]; 
    const char *param = [ jsonString UTF8String ];

    struct _prtn_call_into_go_return result = _prtn_call_into_go((char*)(name), (char*)(param));

    NSString * r0 = result.r0 != NULL ? [NSString stringWithUTF8String:result.r0] : NULL;
    NSString * r1 = result.r1 != NULL ? [NSString stringWithUTF8String:result.r1] : NULL;

    free(result.r0);
    free(result.r1);

    NSString *promiseId = [paramDict objectForKey:@"promiseId"];
    NSString *javaScript = [NSString stringWithFormat:@"_proton_resolvePromise('%@','%@', '%@');",promiseId, r0, r1];
    [self.webView evaluateJavaScript:javaScript completionHandler:nil];
}
@end

AppContext * g_appContext = nil;

@interface AppDelegate : NSObject <NSApplicationDelegate>

@property(nonatomic, retain)  NSWindow  * _Nullable window;
@property(nonatomic, retain)  NSMenu  * _Nullable mainMenu;
@property(nonatomic, retain)  NSStatusItem * _Nullable statusItem;

- (void)applicationWillFinishLaunching:(NSNotification *)notification;
- (void)applicationDidFinishLaunching:(NSNotification *)notification;
@end


@implementation AppDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    NSLog(@"[objc %@] in applicationShouldTerminateAfterLastWindowClosed", [NSThread currentThread]);


	[NSApp stop:nil];   
    
	return FALSE;
}

static NSWindow * createWindow(id appName) {
    NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |  NSWindowStyleMaskResizable;
    NSWindow *window = [[NSWindow alloc]
                        initWithContentRect:NSMakeRect(0, 0, 800, 600)
                        styleMask:style
                        backing:NSBackingStoreBuffered
                        defer:NO];
    [window cascadeTopLeftFromPoint:NSMakePoint(20,20)];
    [window setTitle:appName];
    [window makeKeyAndOrderFront:nil];
    
    return window;
}

static NSMenu *createMainMenu(id appName) {
    id appMenu = [[NSMenu alloc] init];
    [appMenu addItem:[[NSMenuItem alloc]
                      initWithTitle:[@"Quit " stringByAppendingString:appName]
                      action:@selector(terminate:)
                      keyEquivalent:@"q"]];
    
    id appMenuItem = [[NSMenuItem alloc] init];
    [appMenuItem setSubmenu:appMenu];
    
    NSMenu *mainMenu = [[NSMenu alloc] init];
    [mainMenu addItem:appMenuItem];
    
    return mainMenu;
}

static NSStatusItem *createMenuExtra() {
    NSStatusItem * statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    statusItem.button.title= @"☄️";
     
    NSMenu *menu = [[NSMenu alloc] init];
    [menu addItem:[NSMenuItem separatorItem]];
    [menu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"Q"];
    statusItem.menu = menu;
    
    return statusItem;
}

static void registerScriptMessageHandler(WKUserContentController *userContentController,
                                         NSString *name) {
    NSString * javascript = [NSString stringWithFormat:
@"function _proton_%@_invoke(s) {  \n"
@"    var args = arguments;  \n"
@"    var promise = new Promise(function(resolve, reject) {  \n"
@"        var promiseId = _proton_genPromiseSequenceNumber();  \n"
@"        _proton_promises[promiseId] = { resolve, reject };  \n"
@"        window.webkit.messageHandlers.%@.postMessage({ promiseId: promiseId, name: \"%@\", param: Array.prototype.slice.call(args) });  \n"
@"    });  \n"
@"    return promise;  \n"
@"}  \n"
@"  \n"
@"window.proton['%@'] = {  \n"
@"    invoke: _proton_%@_invoke  \n"
@"}  "
,
        name, name, name, name, name];
    
    [userContentController addUserScript:[[WKUserScript alloc] initWithSource:javascript
                                                                injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                                             forMainFrameOnly:YES]];
     [userContentController addScriptMessageHandler:g_appContext name:name];
}

static
void executeJS(WKWebView * webView, NSString * javaScript) {
        [webView evaluateJavaScript:javaScript completionHandler:nil];
}

static WKWebView * createWebView(NSRect frame, id handler) {
    WKWebViewConfiguration *theConfiguration = [[WKWebViewConfiguration alloc] init];
    [theConfiguration.preferences setValue:@YES forKey:@"developerExtrasEnabled"];
    [theConfiguration.preferences setValue:@YES forKey:@"fullScreenEnabled"];
    [theConfiguration.preferences setValue:@YES forKey:@"javaScriptCanAccessClipboard"];
    [theConfiguration.preferences setValue:@YES forKey:@"DOMPasteAllowed"];
    
    WKUserContentController *userContentController = [[WKUserContentController alloc] init];
    theConfiguration.userContentController = userContentController;
    
   
    NSString * script1 =
@"var _proton_promises = {};  "
@"var _proton_promiseSeq = 0;  "
@"window.proton = {};"
@"function _proton_genPromiseSequenceNumber() {  "
@"    function uuidv4() {  "
@"        return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function (c) {  "
@"            var r = Math.random() * 16 | 0, v = c == 'x' ? r : (r & 0x3 | 0x8);  "
@"            return v.toString(16);  "
@"        });  "
@"    }  "
@"    _proton_promiseSeq = _proton_promiseSeq + 1;  "
@"    return _proton_promiseSeq;  "
@"}  "
@"function _proton_resolvePromise(promiseId, data, error) {  "
@"    if (error) {  "
@"        _proton_promises[promiseId].reject(data);  "
@"    } else {  "
@"        _proton_promises[promiseId].resolve(data);  "
@"    }  "
@"    delete _proton_promises[promiseId];  "
@"}  "
;
    [userContentController addUserScript:[[WKUserScript alloc] initWithSource:script1
                                                                injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                                             forMainFrameOnly:YES]];

    // registerScriptMessageHandler(userContentController, name);
    
    WKWebView *webView = [[WKWebView alloc]
                          initWithFrame:frame
                          configuration:theConfiguration];

    return webView;
}

- (void) applicationWillFinishLaunching: (NSNotification *)notification {
    NSLog(@"[objc %@] in applicationWillFinishLaunching", [NSThread currentThread]);
    
    id appName = [[NSProcessInfo processInfo] processName];
    
    self.window = g_appContext.window;
    
    self.mainMenu = createMainMenu(appName);
    [NSApp setMainMenu:self.mainMenu];
    
    self.statusItem = g_appContext.statusItem;
    
    self.window.contentView = g_appContext.webView;
    
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSLog(@"[objc %@] in applicationDidFinishLaunching", [NSThread currentThread]);

    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)  MenuExtraCallback: (NSMenuItem*) sender {
    const char *extractedExpr = [ sender.title UTF8String];
    const char *extractedExpr2 = [[NSString stringWithFormat:@"%ld", sender.tag] UTF8String];
    const char *callbackName = [@"MenuItemCallback" UTF8String];
    _prtn_call_into_go((char*)callbackName, (char*)(extractedExpr2));
}
@end

int prtn_event_loop(void) {
    @autoreleasepool
    {
        AppDelegate* minidelegate = [AppDelegate alloc];
        
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp setDelegate:minidelegate];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    return 0;
}

#ifdef STANDALONEPROG
int main() { return prtn_event_loop(); }
#endif

int prtn_initialize(void) {
    g_appContext = [AppContext alloc];
    g_appContext.window = createWindow(@"my new name");
    g_appContext.statusItem = createMenuExtra();
    g_appContext.webView = createWebView(g_appContext.window.contentView.frame, g_appContext);
    return 0;
}

int prtn_set_title (const char* title) {
    
    [g_appContext.window setTitle:[NSString stringWithUTF8String:title]];
    
    return 0;
}

int prtn_set_menu_extra_text (const char* text) {
    
    g_appContext.statusItem.button.title = [NSString stringWithUTF8String:text];
    
    return 0;
}

int prtn_add_menu_extra_item (const char* text, int tag) {
    NSMenuItem *extractedExpr = [g_appContext.statusItem.menu
        addItemWithTitle:[NSString stringWithUTF8String:text]
                  action:@selector(MenuExtraCallback:)
           keyEquivalent:@""];
    extractedExpr.tag  = tag;
    
    return 0;
}

int prtn_add_content_path (const char* _Nullable path) {
	// NSLog([NSString stringWithUTF8String:path]);

    NSURL *url = [NSURL URLWithString:[NSString stringWithUTF8String:path] ];
    NSString *html = [NSString stringWithContentsOfURL:url encoding:NSUTF8StringEncoding error:nil];
    NSURL *baseUrl = [NSURL URLWithString:@""];
    
    [g_appContext.webView loadHTMLString:html baseURL:baseUrl];
    
    return 0;
}

int prtn_add_script_message_handler(const char * _Nullable name) {
	// NSLog([NSString stringWithUTF8String:name]);

	NSString * ns_name = [NSString stringWithUTF8String:name];
    
    registerScriptMessageHandler(g_appContext.webView.configuration.userContentController, ns_name);

	return 0;
}

int prtn_execute_script(const char * _Nullable script) {
	NSString * ns_script = [NSString stringWithUTF8String:script];
    
    executeJS(g_appContext.webView, ns_script);

	return 0;
}
